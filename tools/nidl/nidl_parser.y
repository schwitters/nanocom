/* Nano-IDL Lemon grammar (CORBA-like subset) with C-like type specs.
 *
 * This grammar is used with SQLite's Lemon parser generator.
 * It parses a "module" containing typedefs, structs, interfaces, and coclasses.
 *
 * Key feature: a small but practical C-like type syntax ("type_spec"):
 *   [const] [unsigned|signed] base [* ...]
 *
 * where base can be:
 *   - long, long long, short, int, char, octet
 *   - IDENT (user-defined type)
 *
 * This enables fields like:  const octet *ptr;
 * and typedefs like:         typedef unsigned long uint32;
 */

%token_type {token_t}              /* Every terminal token's semantic type. */

%include {
/* C code copied verbatim into the generated parser source. */
#include "nidl_lex.h"              /* token_t + TOK_* codes (from Lemon header) */
#include "nidl_ast.h"              /* idl_* AST structs */
#include "nidl_arena.h"            /* arena allocator for stable pointers */
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* Parse context passed into Lemon as %extra_argument.
 * - arena: for all AST allocations and duplicated strings
 * - file:  root AST object to fill
 * - had_error: set to 1 on any error
 */
typedef struct parse_ctx_s {
  arena_t *a;
  idl_file_t *file;
  int had_error;
} parse_ctx_t;

/* Duplicate token text into arena (token text is borrowed from input). */
static char *dup_tok(parse_ctx_t *c, token_t t) { return arena_strdup(c->a, t.ptr, t.len); }

/* Uniform error reporting with line/column and a near-token excerpt. */
static void parse_error(parse_ctx_t *c, token_t t, const char *msg) {
  fprintf(stderr, "IDL parse error at %u:%u: %s near '%.*s'\n", t.line, t.col, msg, (int)t.len, t.ptr);
  c->had_error = 1;
}

/* "Append" helpers for singly-linked lists in idl_file_t.
 * NOTE: We push to front (LIFO). Codegen later usually reverses order
 * when it needs "source order".
 */
static void append_typedef(idl_file_t *f, idl_typedef_t *td){ td->next=f->typedefs; f->typedefs=td; }
static void append_struct(idl_file_t *f, idl_struct_t *st){ st->next=f->structs; f->structs=st; }
static void append_interface(idl_file_t *f, idl_interface_t *it){ it->next=f->interfaces; f->interfaces=it; }
static void append_coclass(idl_file_t *f, idl_coclass_t *cc){ cc->next=f->coclasses; f->coclasses=cc; }

/* "Prepend" helpers for child lists (also LIFO). */
static idl_struct_field_t *prepend_field(idl_struct_field_t *list, idl_struct_field_t *f2){ f2->next=list; return f2; }
static idl_method_t *prepend_method(idl_method_t *list, idl_method_t *m){ m->next=list; return m; }
static idl_param_t *prepend_param(idl_param_t *list, idl_param_t *p){ p->next=list; return p; }

/* Normalize a parsed type_spec into a single string representation.
 * Example:
 *   qual="const", sign="unsigned", base="long long", ptrs=2  -> "const unsigned long long * *"
 *
 * NOTE: The output string is stored in the arena and attached to the AST.
 * Codegen is responsible for mapping/normalizing this for the target language.
 *
 * IMPORTANT: This is a simple formatter; it does not attempt deep C parsing rules
 * (like "unsigned" applying only to integer types, etc.). It matches what the grammar accepts.
 */
static char *make_type(parse_ctx_t *ctx, const char *qual, const char *sign, const char *base, int ptrs)
{
  char buf[256];
  buf[0] = 0;
  if (qual && qual[0]) { strncat(buf, qual, sizeof(buf)-1); strncat(buf, " ", sizeof(buf)-1); }
  if (sign && sign[0]) { strncat(buf, sign, sizeof(buf)-1); strncat(buf, " ", sizeof(buf)-1); }
  if (base && base[0]) { strncat(buf, base, sizeof(buf)-1); }
  for (int i=0;i<ptrs;i++) { strncat(buf, " *", sizeof(buf)-1); } /* pointer levels */
  return arena_strdup(ctx->a, buf, (size_t)strlen(buf));
}
}

%extra_argument {parse_ctx_t *ctx} /* passed to Parse() as the extra argument */

%syntax_error   { parse_error(ctx, TOKEN, "syntax error"); } /* TOKEN is Lemon's current token */
%parse_failure  { ctx->had_error = 1; }                      /* unrecoverable parse failure */
%stack_overflow { ctx->had_error = 1; }                      /* parser stack overflow */

%token_prefix TOK_                /* Lemon will generate token constants like TOK_MODULE, TOK_IDENT, ... */

/* === Nonterminal semantic types === */
%type uuid_opt {char*}            /* optional uuid string (or NULL) */
%type doc_opt  {char*}            /* optional doc string (or NULL) */
%type ident    {char*}            /* identifier text */
%type type_spec {char*}           /* normalized type string */
%type qual_opt {const char*}      /* "const" or NULL */
%type sign_opt {const char*}      /* "unsigned"/"signed" or NULL */
%type base_type {char*}           /* base type text (keyword or IDENT) */
%type ptr_chain {int}             /* number of '*' levels */
%type opt_base {char*}            /* optional base interface name */

/* AST nodes returned by productions. */
%type typedef_decl {idl_typedef_t*}
%type struct_decl  {idl_struct_t*}
%type field_list   {idl_struct_field_t*}
%type field_decl   {idl_struct_field_t*}

%type interface_decl {idl_interface_t*}
%type method_list    {idl_method_t*}
%type method_decl    {idl_method_t*}
%type param_list_opt {idl_param_t*}
%type param_list     {idl_param_t*}
%type param_decl     {idl_param_t*}
%type opt_optional   {int}

%type coclass_decl {idl_coclass_t*}

/* ============================================================================
 * Entry point
 * ----------------------------------------------------------------------------
 * A complete file is:
 *   module <ident> { <decl_list> };
 */
start ::= MODULE ident(M) LBRACE decl_list RBRACE SEMI.
{
  /* Store the module name on the root AST file node. */
  ctx->file->module_name = M;
}

/* ============================================================================
 * Declaration list
 * ----------------------------------------------------------------------------
 * We parse an unbounded list of declarations inside module braces.
 */
decl_list ::= .                         /* empty list */
decl_list ::= decl_list decl.           /* append one more decl */

/* Each decl is optionally preceded by doc comments (doc_opt).
 * We attach doc strings directly onto the AST node.
 */
decl ::= doc_opt(D) typedef_decl(T). { T->doc = D; append_typedef(ctx->file, T); }
decl ::= doc_opt(D) struct_decl(S).  { S->doc = D; append_struct(ctx->file, S); }

/* Interface decl: optional doc + optional uuid attribute. */
decl ::= doc_opt(D) uuid_opt(U) INTERFACE interface_decl(I).
{
  I->doc = D;
  I->uuid = U;
  append_interface(ctx->file, I);
}

/* Coclass decl: optional doc + optional uuid attribute. */
decl ::= doc_opt(D) uuid_opt(U) COCLASS coclass_decl(C).
{
  C->doc = D;
  C->uuid = U;
  append_coclass(ctx->file, C);
}

/* ============================================================================
 * Attributes
 * ----------------------------------------------------------------------------
 * uuid_opt matches the CORBA/COM-style attribute:
 *   [uuid("...")]
 *
 * NOTE: We purposely accept IDENT as the attribute name and assume it is "uuid"
 * at a higher level (lexer can enforce or you can extend grammar to require UUID).
 */
uuid_opt(U) ::= . { U = NULL; }
uuid_opt(U) ::= LBRACKET IDENT LPAREN STRING(S) RPAREN RBRACKET.
{
  U = dup_tok(ctx, S);  /* stores the raw UUID string */
}

/* doc_opt attaches documentation comments.
 * The lexer recognizes:
 *   - /// line docs
 *   - /** block docs * /
 * and returns a DOC token with markers stripped.
 */
doc_opt(D) ::= .        { D = NULL; }
doc_opt(D) ::= DOC(T).  { D = dup_tok(ctx, T); }

/* ============================================================================
 * Identifier
 * ----------------------------------------------------------------------------
 * IDENT is a lexer token; ident() duplicates it into the arena.
 */
ident(I) ::= IDENT(T). { I = dup_tok(ctx, T); }

/* ============================================================================
 * Type specifier
 * ----------------------------------------------------------------------------
 * type_spec := qual_opt sign_opt base_type ptr_chain
 *
 * Example accepted strings:
 *   const octet *             -> qual="const", sign=NULL, base="octet", ptrs=1
 *   unsigned long long **     -> sign="unsigned", base="long long", ptrs=2
 *
 * NOTE: We allow "long long" by a two-token production: LONG LONG.
 */
qual_opt(Q) ::= .      { Q = NULL; }
qual_opt(Q) ::= CONST. { Q = "const"; }

sign_opt(S) ::= .         { S = NULL; }
sign_opt(S) ::= UNSIGNED. { S = "unsigned"; }
sign_opt(S) ::= SIGNED.   { S = "signed"; }

/* base_type returns the canonical base type string. */
base_type(B) ::= LONG LONG.  { B = "long long"; }
base_type(B) ::= LONG.       { B = "long"; }
base_type(B) ::= SHORT.      { B = "short"; }
base_type(B) ::= INT_KW.     { B = "int"; }
base_type(B) ::= CHAR_KW.    { B = "char"; }
base_type(B) ::= OCTET.      { B = "octet"; }
base_type(B) ::= IDENT(T).   { B = dup_tok(ctx, T); } /* user-defined type */

/* ptr_chain counts pointer stars; it is left-recursive to accumulate. */
ptr_chain(P) ::= .                       { P = 0; }
ptr_chain(P) ::= ptr_chain(P0) STAR.     { P = P0 + 1; }

/* Combine the parts into a single normalized string stored in the arena. */
type_spec(TS) ::= qual_opt(Q) sign_opt(S) base_type(B) ptr_chain(P).
{
  TS = make_type(ctx, Q, S, B, P);
}

/* ============================================================================
 * Typedef
 * ----------------------------------------------------------------------------
 * typedef <type_spec> <alias>;
 */
typedef_decl(TD) ::= TYPEDEF type_spec(TGT) ident(ALIAS) SEMI.
{
  TD = (idl_typedef_t*)arena_alloc(ctx->a, sizeof(*TD));
  TD->target = TGT;
  TD->alias  = ALIAS;
}

/* ============================================================================
 * Struct
 * ----------------------------------------------------------------------------
 * struct <name> { <field_list> };
 */
struct_decl(ST) ::= STRUCT ident(N) LBRACE field_list(FL) RBRACE SEMI.
{
  ST = (idl_struct_t*)arena_alloc(ctx->a, sizeof(*ST));
  ST->name   = N;
  ST->fields = FL; /* list is built via prepend_field (LIFO) */
}

/* Field list: zero or more field declarations. */
field_list(FL) ::= . { FL = NULL; }
field_list(FL) ::= field_list(F0) field_decl(F1). { FL = prepend_field(F0, F1); }

/* Field: optional doc + type_spec + name + ';' */
field_decl(FD) ::= doc_opt(D) type_spec(T) ident(N) SEMI.
{
  FD = (idl_struct_field_t*)arena_alloc(ctx->a, sizeof(*FD));
  FD->doc  = D;
  FD->type = T;
  FD->name = N;
}

/* ============================================================================
 * Interface
 * ----------------------------------------------------------------------------
 * [uuid] interface <name> (: <base>)? { <methods> };
 */
interface_decl(IT) ::= ident(N) opt_base(B) LBRACE method_list(ML) RBRACE SEMI.
{
  IT = (idl_interface_t*)arena_alloc(ctx->a, sizeof(*IT));
  IT->name    = N;
  IT->base    = B;   /* optional base interface name */
  IT->methods = ML;  /* list built via prepend_method (LIFO) */
}

/* Optional base interface: ": i_unknown" etc. */
opt_base(B) ::= .                { B = NULL; }
opt_base(B) ::= COLON ident(N).  { B = N; }

/* Method list: zero or more methods. */
method_list(ML) ::= . { ML = NULL; }
method_list(ML) ::= method_list(M0) method_decl(M1). { ML = prepend_method(M0, M1); }

/* Method: optional doc + return type + name + "(" params? ")" ";" */
method_decl(MD) ::= doc_opt(D) type_spec(RT) ident(N) LPAREN param_list_opt(PL) RPAREN SEMI.
{
  MD = (idl_method_t*)arena_alloc(ctx->a, sizeof(*MD));
  MD->doc      = D;
  MD->ret_type = RT;
  MD->name     = N;
  MD->params   = PL; /* list built via prepend_param (LIFO) */
}

/* Optional parameter list. */
param_list_opt(PL) ::= .                 { PL = NULL; }
param_list_opt(PL) ::= param_list(P0).   { PL = P0; }

/* Param list is comma-separated. */
param_list(PL) ::= param_decl(P). { PL = P; }
param_list(PL) ::= param_list(P0) COMMA param_decl(P1). { PL = prepend_param(P0, P1); }

/* Optional [optional] attribute on parameters.
 * NOTE: This is intentionally generic: it accepts any IDENT inside [...]
 * and treats it as "optional". You may tighten this to require IDENT == "optional".
 */
opt_optional(O) ::= .                        { O = 0; }
opt_optional(O) ::= LBRACKET IDENT RBRACKET. { O = 1; }

/* Parameter directions: in/out/inout plus optional docs and [optional] flag.
 * Parameter syntax:
 *   [doc] [optional] in    <type_spec> <name>
 *   [doc] [optional] out   <type_spec> <name>
 *   [doc] [optional] inout <type_spec> <name>
 */
param_decl(P) ::= doc_opt(D) opt_optional(O) IN type_spec(T) ident(N).
{
  P = (idl_param_t*)arena_alloc(ctx->a, sizeof(*P));
  P->doc      = D;
  P->optional = O;
  P->dir      = "in";
  P->type     = T;
  P->name     = N;
}
param_decl(P) ::= doc_opt(D) opt_optional(O) OUT type_spec(T) ident(N).
{
  P = (idl_param_t*)arena_alloc(ctx->a, sizeof(*P));
  P->doc      = D;
  P->optional = O;
  P->dir      = "out";
  P->type     = T;
  P->name     = N;
}
param_decl(P) ::= doc_opt(D) opt_optional(O) INOUT type_spec(T) ident(N).
{
  P = (idl_param_t*)arena_alloc(ctx->a, sizeof(*P));
  P->doc      = D;
  P->optional = O;
  P->dir      = "inout";
  P->type     = T;
  P->name     = N;
}

/* ============================================================================
 * Coclass
 * ----------------------------------------------------------------------------
 * coclass <name> { };
 *
 * NOTE: Right now, coclass body is empty. You can extend this to list implemented
 * interfaces, factories, activation rules, etc.
 */
coclass_decl(CC) ::= ident(N) LBRACE RBRACE SEMI.
{
  CC = (idl_coclass_t*)arena_alloc(ctx->a, sizeof(*CC));
  CC->name = N;
}