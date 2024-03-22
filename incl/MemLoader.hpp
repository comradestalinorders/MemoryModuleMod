#pragma once
#include "MemoryModule.h"
class MemLoader
{
private:
	MemLoader();
public:
	virtual ~MemLoader();

	/**
	 * Load EXE/DLL from memory location with the given size.
	 * All dependencies are resolved using default LoadLibrary/GetProcAddress
	 * calls through the Windows API.
	 */
	static HMEMORYMODULE MemLoadLibrary(const void *, size_t);

	/**
	 * Load EXE/DLL from memory location with the given size using custom dependency
	 * resolvers.
	 * Dependencies will be resolved using passed callback methods.
	 */
	static HMEMORYMODULE MemLoadLibraryEx(const void *, size_t,
		CustomAllocFunc,
		CustomFreeFunc,
		CustomLoadLibraryFunc,
		CustomGetProcAddressFunc,
		CustomFreeLibraryFunc,
		void *);

	/**
	 * Get address of exported method. Supports loading both by name and by
	 * ordinal value.
	 */
	static FARPROC MemGetProcAddress(HMEMORYMODULE, LPCSTR);

	/**
	 * Free previously loaded EXE/DLL.
	 */
	static void MemFreeLibrary(HMEMORYMODULE);

	/**
	 * Execute entry point (EXE only). The entry point can only be executed
	 * if the EXE has been loaded to the correct base address or it could
	 * be relocated (i.e. relocation information have not been stripped by
	 * the linker).
	 * Important: calling this function will not return, i.e. once the loaded
	 * EXE finished running, the process will terminate.
	 * Returns a negative value if the entry point could not be executed.
	 */
	static int MemCallEntryPoint(HMEMORYMODULE);

	/**
	 * Find the location of a resource with the specified type and name.
	 */
	static HMEMORYRSRC MemFindResource(HMEMORYMODULE, LPCTSTR, LPCTSTR);

	/**
	 * Find the location of a resource with the specified type, name and language.
	 */
	static HMEMORYRSRC MemFindResourceEx(HMEMORYMODULE, LPCTSTR, LPCTSTR, WORD);

	/**
	 * Get the size of the resource in bytes.
	 */
	static DWORD MemSizeofResource(HMEMORYMODULE, HMEMORYRSRC);

	/**
	 * Get a pointer to the contents of the resource.
	 */
	static LPVOID MemLoadResource(HMEMORYMODULE, HMEMORYRSRC);

	/**
	 * Load a string resource.
	 */
	static int  MemLoadString(HMEMORYMODULE, UINT, LPTSTR, int);

	/**
	 * Load a string resource with a given language.
	 */
	static int MemLoadStringEx(HMEMORYMODULE, UINT, LPTSTR, int, WORD);

	/**
	* Default implementation of CustomAllocFunc that calls VirtualAlloc
	* internally to allocate memory for a library
	* This is the default as used by MemoryLoadLibrary.
	*/
	static LPVOID MemDefaultAlloc(LPVOID, SIZE_T, DWORD, DWORD, void *);

	/**
	* Default implementation of CustomFreeFunc that calls VirtualFree
	* internally to free the memory used by a library
	* This is the default as used by MemoryLoadLibrary.
	*/
	static BOOL MemDefaultFree(LPVOID, SIZE_T, DWORD, void *);

	/**
	 * Default implementation of CustomLoadLibraryFunc that calls LoadLibraryA
	 * internally to load an additional libary.
	 * This is the default as used by MemoryLoadLibrary.
	 */
	static HCUSTOMMODULE MemDefaultLoadLibrary(LPCSTR, void *);

	/**
	 * Default implementation of CustomGetProcAddressFunc that calls GetProcAddress
	 * internally to get the address of an exported function.
	 * This is the default as used by MemoryLoadLibrary.
	 */
	static FARPROC MemDefaultGetProcAddress(HCUSTOMMODULE, LPCSTR, void *);

	/**
	 * Default implementation of CustomFreeLibraryFunc that calls FreeLibrary
	 * internally to release an additional libary.
	 * This is the default as used by MemoryLoadLibrary.
	 */
	static void MemDefaultFreeLibrary(HCUSTOMMODULE, void *);
};

