#pragma once

#include <ncom/base.h>

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @def NCOM_EXPORT
	 * @brief Platform-independent macro to export symbols from a dynamic library.
	 *
	 * This macro ensures that the annotated function is exported by the compiler
	 * and linker, making it visible and callable from outside the shared library
	 * (e.g., a DLL on Windows, or an SO on Linux/POSIX).
	 *
	 * @note On Windows, this translates to __declspec(dllexport).
	 * On GCC/Clang, it uses __attribute__((visibility("default"))).
	 * It is highly recommended to compile POSIX libraries with the
	 * `-fvisibility=hidden` flag so that *only* symbols marked with
	 * NCOM_EXPORT are exposed.
	 */
#if defined(_WIN32) || defined(__CYGWIN__)
#define NCOM_EXPORT __declspec(dllexport)
#else
#if defined(__GNUC__) && __GNUC__ >= 4
#define NCOM_EXPORT __attribute__((visibility("default")))
#else
#define NCOM_EXPORT
#endif
#endif

	 /**
	  * @def NCOM_PLUGIN_GET_API_V1_SYMBOL
	  * @brief The exact string literal of the exported entry point symbol.
	  *
	  * The host application uses this string to resolve the entry point
	  * via dlsym() (POSIX) or GetProcAddress() (Windows).
	  */
#define NCOM_PLUGIN_GET_API_V1_SYMBOL "ncom_plugin_get_api_v1"

	  /* ============================================================================
	   * Plugin API Signatures
	   * ============================================================================ */

	   /**
		* @brief Optional initialization hook called immediately after the plugin is loaded.
		*
		* Plugins can use this hook to allocate global resources, start background
		* threads, or initialize third-party libraries.
		*/
	typedef void (*ncom_plugin_init_fn)(void);

	/**
	 * @brief Optional shutdown hook called right before the plugin is unloaded.
	 *
	 * Plugins should use this hook to join threads, free global resources,
	 * and ensure all outstanding handles are closed.
	 */
	typedef void (*ncom_plugin_shutdown_fn)(void);

	/**
	 * @brief Factory function signature for creating component instances.
	 *
	 * @param clsid The Class ID (CLSID) of the component to create.
	 * @param iid   The Interface ID (IID) requested from the newly created instance.
	 * @param out   Receives the requested interface pointer on success (AddRef'ed).
	 *
	 * @return ncom_status_t NCOM_OK on success, NCOM_E_NOT_FOUND if the CLSID
	 * is unknown, or an appropriate error code.
	 */
	typedef ncom_status_t(*ncom_plugin_create_instance_fn)(
		const ncom_clsid_t* clsid,
		const ncom_iid_t* iid,
		void** out
		);

	/**
	 * @brief The v1 API function table exported by an ncom plugin.
	 *
	 * This table provides the host with the necessary function pointers
	 * to interact with the plugin and instantiate its components without
	 * knowing any of its internal implementation details.
	 */
	typedef struct {
		/**
		 * @brief ABI version of this structure. MUST be set to 1.
		 */
		uint32_t abi_version;

		/**
		 * @brief Pointer to the instance creation function. MUST NOT be NULL.
		 */
		ncom_plugin_create_instance_fn create_instance;

		/**
		 * @brief Pointer to the initialization function. May be NULL.
		 */
		ncom_plugin_init_fn plugin_init;

		/**
		 * @brief Pointer to the shutdown function. May be NULL.
		 */
		ncom_plugin_shutdown_fn plugin_shutdown;
	} ncom_plugin_api_v1_t;

	/**
	 * @brief The signature of the single exported entry point function.
	 *
	 * Every valid ncom plugin MUST implement a function matching this signature,
	 * name it exactly as defined in NCOM_PLUGIN_GET_API_V1_SYMBOL, and export
	 * it using the NCOM_EXPORT macro.
	 *
	 * @return const ncom_plugin_api_v1_t* A pointer to the static API table.
	 */
	typedef const ncom_plugin_api_v1_t* (*ncom_plugin_get_api_v1_fn)(void);

#ifdef __cplusplus
}
#endif