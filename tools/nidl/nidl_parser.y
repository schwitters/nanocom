/* Nano-IDL Lemon grammar (CORBA-like subset) with C-like type specs.
   type_spec:
     [const] [unsigned|signed] base [* ...]
   base:
     long, short, int, char, octet, IDENT, and "long long"

   Also allows struct fields like: const octet *ptr;
*/

%token_type {token_t}

%include {
#include "nidl_lex.h"
#include "nidl_ast.h"
#include "nidl_arena.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

typedef struct parse_ctx_s {
  arena_t *a;
  idl_file_t *file;
  int had_error;
} parse_ctx_t;

static char *dup_tok(parse_ctx_t *c, token_t t) { return arena_strdup(c->a, t.ptr, t.len); }

static void parse_error(parse_ctx_t *c, token_t t, const char *msg) {
  fprintf(stderr, "IDL parse error at %u:%u: %s near '%.*s'\n", t.line, t.col, msg, (int)t.len, t.ptr);
  c->had_error = 1;
}

static void append_typedef(idl_file_t *f, idl_typedef_t *td){ td->next=f->typedefs; f->typedefs=td; }
static void append_struct(idl_file_t *f, idl_struct_t *st){ st->next=f->structs; f->structs=st; }
static void append_interface(idl_file_t *f, idl_interface_t *it){ it->next=f->interfaces; f->interfaces=it; }
static void append_coclass(idl_file_t *f, idl_coclass_t *cc){ cc->next=f->coclasses; f->coclasses=cc; }

static idl_struct_field_t *prepend_field(idl_struct_field_t *list, idl_struct_field_t *f2){ f2->next=list; return f2; }
static idl_method_t *prepend_method(idl_method_t *list, idl_method_t *m){ m->next=list; return m; }
static idl_param_t *prepend_param(idl_param_t *list, idl_param_t *p){ p->next=list; return p; }

static char *make_type(parse_ctx_t *ctx, const char *qual, const char *sign, const char *base, int ptrs)
{
  char buf[256];
  buf[0] = 0;
  if (qual && qual[0]) { strncat(buf, qual, sizeof(buf)-1); strncat(buf, " ", sizeof(buf)-1); }
  if (sign && sign[0]) { strncat(buf, sign, sizeof(buf)-1); strncat(buf, " ", sizeof(buf)-1); }
  if (base && base[0]) { strncat(buf, base, sizeof(buf)-1); }
  for (int i=0;i<ptrs;i++) { strncat(buf, " *", sizeof(buf)-1); }
  return arena_strdup(ctx->a, buf, (size_t)strlen(buf));
}
}

%extra_argument {parse_ctx_t *ctx}

%syntax_error { parse_error(ctx, TOKEN, "syntax error"); }
%parse_failure { ctx->had_error = 1; }
%stack_overflow { ctx->had_error = 1; }

%token_prefix TOK_

%type uuid_opt {char*}
%type doc_opt {char*}
%type ident {char*}
%type type_spec {char*}
%type qual_opt {const char*}
%type sign_opt {const char*}
%type base_type {char*}
%type ptr_chain {int}
%type opt_base {char*}

%type typedef_decl {idl_typedef_t*}
%type struct_decl {idl_struct_t*}
%type field_list {idl_struct_field_t*}
%type field_decl {idl_struct_field_t*}

%type interface_decl {idl_interface_t*}
%type method_list {idl_method_t*}
%type method_decl {idl_method_t*}
%type param_list_opt {idl_param_t*}
%type param_list {idl_param_t*}
%type param_decl {idl_param_t*}
%type opt_optional {int}

%type coclass_decl {idl_coclass_t*}

/* Entry */
start ::= MODULE ident(M) LBRACE decl_list RBRACE SEMI.
{
  ctx->file->module_name = M;
}

decl_list ::= .
decl_list ::= decl_list decl.

decl ::= doc_opt(D) typedef_decl(T). { T->doc = D; append_typedef(ctx->file, T); }
decl ::= doc_opt(D) struct_decl(S).  { S->doc = D; append_struct(ctx->file, S); }

decl ::= doc_opt(D) uuid_opt(U) INTERFACE interface_decl(I).
{
  I->doc = D;
  I->uuid = U;
  append_interface(ctx->file, I);
}

decl ::= doc_opt(D) uuid_opt(U) COCLASS coclass_decl(C).
{
  C->doc = D;
  C->uuid = U;
  append_coclass(ctx->file, C);
}

/* [uuid("...")] */
uuid_opt(U) ::= . { U = NULL; }
uuid_opt(U) ::= LBRACKET IDENT LPAREN STRING(S) RPAREN RBRACKET.
{
  U = dup_tok(ctx, S);
}


/* doc comments: ///... or /<NOSPACE>**...*<NOSPACE>/ (lexer strips markers) */
doc_opt(D) ::= . { D = NULL; }
doc_opt(D) ::= DOC(T). { D = dup_tok(ctx, T); }

/* ident */
ident(I) ::= IDENT(T). { I = dup_tok(ctx, T); }

/* type_spec */
qual_opt(Q) ::= . { Q = NULL; }
qual_opt(Q) ::= CONST. { Q = "const"; }

sign_opt(S) ::= . { S = NULL; }
sign_opt(S) ::= UNSIGNED. { S = "unsigned"; }
sign_opt(S) ::= SIGNED. { S = "signed"; }

base_type(B) ::= LONG LONG. { B = "long long"; }
base_type(B) ::= LONG. { B = "long"; }
base_type(B) ::= SHORT. { B = "short"; }
base_type(B) ::= INT_KW. { B = "int"; }
base_type(B) ::= CHAR_KW. { B = "char"; }
base_type(B) ::= OCTET. { B = "octet"; }
base_type(B) ::= IDENT(T). { B = dup_tok(ctx, T); }

ptr_chain(P) ::= . { P = 0; }
ptr_chain(P) ::= ptr_chain(P0) STAR. { P = P0 + 1; }

type_spec(TS) ::= qual_opt(Q) sign_opt(S) base_type(B) ptr_chain(P).
{
  TS = make_type(ctx, Q, S, B, P);
}

/* typedef <type> <alias>; */
typedef_decl(TD) ::= TYPEDEF type_spec(TGT) ident(ALIAS) SEMI.
{
  TD = (idl_typedef_t*)arena_alloc(ctx->a, sizeof(*TD));
  TD->target = TGT;
  TD->alias = ALIAS;
}

/* struct name { fields } ; */
struct_decl(ST) ::= STRUCT ident(N) LBRACE field_list(FL) RBRACE SEMI.
{
  ST = (idl_struct_t*)arena_alloc(ctx->a, sizeof(*ST));
  ST->name = N;
  ST->fields = FL;
}

field_list(FL) ::= . { FL = NULL; }
field_list(FL) ::= field_list(F0) field_decl(F1). { FL = prepend_field(F0, F1); }

field_decl(FD) ::= doc_opt(D) type_spec(T) ident(N) SEMI.
{
  FD = (idl_struct_field_t*)arena_alloc(ctx->a, sizeof(*FD));
  FD->doc = D;
  FD->type = T;
  FD->name = N;
}

/* interface name (: base)? { methods } ; */
interface_decl(IT) ::= ident(N) opt_base(B) LBRACE method_list(ML) RBRACE SEMI.
{
  IT = (idl_interface_t*)arena_alloc(ctx->a, sizeof(*IT));
  IT->name = N;
  IT->base = B;
  IT->methods = ML;
}

opt_base(B) ::= . { B = NULL; }
opt_base(B) ::= COLON ident(N). { B = N; }

method_list(ML) ::= . { ML = NULL; }
method_list(ML) ::= method_list(M0) method_decl(M1). { ML = prepend_method(M0, M1); }

method_decl(MD) ::= doc_opt(D) type_spec(RT) ident(N) LPAREN param_list_opt(PL) RPAREN SEMI.
{
  MD = (idl_method_t*)arena_alloc(ctx->a, sizeof(*MD));
  MD->doc = D;
  MD->ret_type = RT;
  MD->name = N;
  MD->params = PL;
}

param_list_opt(PL) ::= . { PL = NULL; }
param_list_opt(PL) ::= param_list(P0). { PL = P0; }

param_list(PL) ::= param_decl(P). { PL = P; }
param_list(PL) ::= param_list(P0) COMMA param_decl(P1). { PL = prepend_param(P0, P1); }

opt_optional(O) ::= . { O = 0; }
opt_optional(O) ::= LBRACKET IDENT RBRACKET. { O = 1; }

param_decl(P) ::= doc_opt(D) opt_optional(O) IN type_spec(T) ident(N).
{
  P = (idl_param_t*)arena_alloc(ctx->a, sizeof(*P));
  P->doc = D;
  P->optional = O;
  P->dir = "in";
  P->type = T;
  P->name = N;
}
param_decl(P) ::= doc_opt(D) opt_optional(O) OUT type_spec(T) ident(N).
{
  P = (idl_param_t*)arena_alloc(ctx->a, sizeof(*P));
  P->doc = D;
  P->optional = O;
  P->dir = "out";
  P->type = T;
  P->name = N;
}
param_decl(P) ::= doc_opt(D) opt_optional(O) INOUT type_spec(T) ident(N).
{
  P = (idl_param_t*)arena_alloc(ctx->a, sizeof(*P));
  P->doc = D;
  P->optional = O;
  P->dir = "inout";
  P->type = T;
  P->name = N;
}

/* coclass name { } ; */
coclass_decl(CC) ::= ident(N) LBRACE RBRACE SEMI.
{
  CC = (idl_coclass_t*)arena_alloc(ctx->a, sizeof(*CC));
  CC->name = N;
}
