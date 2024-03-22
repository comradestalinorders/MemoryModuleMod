#include "../incl/MemLoader.hpp"

MemLoader::MemLoader(){}
MemLoader::~MemLoader() {}


/**
 * Load EXE/DLL from memory location with the given size.
 * All dependencies are resolved using default LoadLibrary/GetProcAddress
 * calls through the Windows API.
 */
HMEMORYMODULE MemLoader::MemLoadLibrary(const void * p, size_t s) {
	return MemoryLoadLibrary(p, s);
}

/**
 * Load EXE/DLL from memory location with the given size using custom dependency
 * resolvers.
 * Dependencies will be resolved using passed callback methods.
 */
HMEMORYMODULE MemLoader::MemLoadLibraryEx(const void *data, size_t size,
	CustomAllocFunc allocMemory,
	CustomFreeFunc freeMemory,
	CustomLoadLibraryFunc loadLibrary,
	CustomGetProcAddressFunc getProcAddress,
	CustomFreeLibraryFunc freeLibrary,
	void *userdata) {
	return MemoryLoadLibraryEx(data, size, allocMemory, freeMemory, loadLibrary, getProcAddress, freeLibrary, userdata);
}

/**
 * Get address of exported method. Supports loading both by name and by
 * ordinal value.
 */
FARPROC MemLoader::MemGetProcAddress(HMEMORYMODULE handle, LPCSTR name) {
	return MemoryGetProcAddress(handle, name);
}

/**
 * Free previously loaded EXE/DLL.
 */
void MemLoader::MemFreeLibrary(HMEMORYMODULE handle) {
	MemoryFreeLibrary(handle);
}

/**
 * Execute entry point (EXE only). The entry point can only be executed
 * if the EXE has been loaded to the correct base address or it could
 * be relocated (i.e. relocation information have not been stripped by
 * the linker).
 * Important: calling this function will not return, i.e. once the loaded
 * EXE finished running, the process will terminate.
 *
 * Returns a negative value if the entry point could not be executed.
 */
int MemLoader:: MemCallEntryPoint(HMEMORYMODULE handle) {
	return MemoryCallEntryPoint(handle);
}

/**
 * Find the location of a resource with the specified type and name.
 */
HMEMORYRSRC MemLoader::MemFindResource(HMEMORYMODULE handle, LPCTSTR name, LPCTSTR type) {
	return MemoryFindResource(handle, name, type);
}

/**
 * Find the location of a resource with the specified type, name and language.
 */
HMEMORYRSRC MemLoader:: MemFindResourceEx(HMEMORYMODULE handle, LPCTSTR name, LPCTSTR type, WORD language) {
	return MemoryFindResourceEx( handle,  name,  type,  language);
}

/**
 * Get the size of the resource in bytes.
 */
DWORD MemLoader:: MemSizeofResource(HMEMORYMODULE handle, HMEMORYRSRC resource) {
	return MemorySizeofResource(handle,  resource);
}

/**
 * Get a pointer to the contents of the resource.
 */
LPVOID MemLoader::MemLoadResource(HMEMORYMODULE handle, HMEMORYRSRC resource) {
	return MemoryLoadResource(handle, resource);
}

/**
 * Load a string resource.
 */
int MemLoader::MemLoadString(HMEMORYMODULE handle, UINT id, LPTSTR buffer, int maxsize) {
	return MemoryLoadString(handle, id, buffer, maxsize);
}

/**
 * Load a string resource with a given language.
 */
int MemLoader::MemLoadStringEx(HMEMORYMODULE handle, UINT id, LPTSTR buffer, int maxsize, WORD language) {
	return MemoryLoadStringEx(handle, id, buffer, maxsize, language);
}

/**
* Default implementation of CustomAllocFunc that calls VirtualAlloc
* internally to allocate memory for a library
*
* This is the default as used by MemoryLoadLibrary.
*/
LPVOID MemLoader::MemDefaultAlloc(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect, void* userdata){
	return MemoryDefaultAlloc(address, size, allocationType, protect, userdata);
}

/**
* Default implementation of CustomFreeFunc that calls VirtualFree
* internally to free the memory used by a library
*
* This is the default as used by MemoryLoadLibrary.
*/
BOOL MemLoader:: MemDefaultFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType, void* userdata) {
	return MemoryDefaultFree(lpAddress, dwSize, dwFreeType, userdata);
}

/**
 * Default implementation of CustomLoadLibraryFunc that calls LoadLibraryA
 * internally to load an additional libary.
 *
 * This is the default as used by MemoryLoadLibrary.
 */
HCUSTOMMODULE MemLoader::MemDefaultLoadLibrary(LPCSTR filename, void *userdata) {
	return  MemoryDefaultLoadLibrary( filename, userdata);
}

/**
 * Default implementation of CustomGetProcAddressFunc that calls GetProcAddress
 * internally to get the address of an exported function.
 *
 * This is the default as used by MemoryLoadLibrary.
 */
FARPROC MemLoader:: MemDefaultGetProcAddress(HCUSTOMMODULE handle, LPCSTR name, void *userdata) {
	return MemoryDefaultGetProcAddress(handle, name, userdata);
}

/**
 * Default implementation of CustomFreeLibraryFunc that calls FreeLibrary
 * internally to release an additional libary.
 *
 * This is the default as used by MemoryLoadLibrary.
 */
void MemLoader:: MemDefaultFreeLibrary(HCUSTOMMODULE handle,  void *userdata) {
	MemoryDefaultFreeLibrary(handle, userdata);
}