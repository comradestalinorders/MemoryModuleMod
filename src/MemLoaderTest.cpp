#include "../incl/MemLoaderTest.h"

void  MemLoaderTest::LoadFromFile(LPCWSTR dllFile)
{
	myfun addNumber;
	HRSRC resourceInfo;
	DWORD resourceSize;
	LPVOID resourceData;
	TCHAR buffer[100];

	HINSTANCE handle = LoadLibrary(dllFile);
	if (handle == NULL)
		return;

	addNumber = (myfun)GetProcAddress(handle, "myfun");
	_tprintf(_T("From file: %d\n"), addNumber());

	resourceInfo = FindResource(handle, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
	_tprintf(_T("FindResource returned 0x%p\n"), resourceInfo);

	resourceSize = SizeofResource(handle, resourceInfo);
	resourceData = LoadResource(handle, resourceInfo);
	_tprintf(_T("Resource data: %ld bytes at 0x%p\n"), resourceSize, resourceData);

	LoadString(handle, 1, buffer, sizeof(buffer));
	_tprintf(_T("String1: %s\n"), buffer);

	LoadString(handle, 20, buffer, sizeof(buffer));
	_tprintf(_T("String2: %s\n"), buffer);

	FreeLibrary(handle);
}

void * MemLoaderTest::ReadLibrary(size_t* pSize, LPCWSTR dllFile) {
	size_t read;
	void* result;
	FILE* fp;

	fp = _tfopen(dllFile, _T("rb"));
	if (fp == NULL)
	{
		_tprintf(_T("Can't open DLL file \"%s\"."), dllFile);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	*pSize = static_cast<size_t>(ftell(fp));
	if (*pSize == 0)
	{
		fclose(fp);
		return NULL;
	}

	result = (unsigned char *)malloc(*pSize);
	if (result == NULL)
	{
		return NULL;
	}

	fseek(fp, 0, SEEK_SET);
	read = fread(result, 1, *pSize, fp);
	fclose(fp);
	if (read != *pSize)
	{
		free(result);
		return NULL;
	}

	return result;
}

void MemLoaderTest:: LoadFromMemory(LPCWCHAR dllFile)
{
	void *data;
	size_t size;
	HMEMORYMODULE handle;
	myfun addNumber;
	HMEMORYRSRC resourceInfo;
	DWORD resourceSize;
	LPVOID resourceData;
	TCHAR buffer[100];


	data = ReadLibrary(&size, dllFile);
	if (data == NULL)
	{
		return;
	}

	handle = MemLoader::MemLoadLibrary(data, size);
	if (handle == NULL)
	{
		_tprintf(_T("Can't load library from memory.\n"));
		goto exit;
	}

	addNumber = (myfun)MemLoader::MemGetProcAddress(handle, "myfun");
	_tprintf(_T("From memory: %d\n"), addNumber());

	resourceInfo = MemLoader::MemFindResource(handle, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
	_tprintf(_T("MemoryFindResource returned 0x%p\n"), resourceInfo);

	resourceSize = MemLoader::MemSizeofResource(handle, resourceInfo);
	resourceData = MemLoader::MemLoadResource(handle, resourceInfo);
	_tprintf(_T("Memory resource data: %ld bytes at 0x%p\n"), resourceSize, resourceData);

	MemLoader::MemLoadString(handle, 1, buffer, sizeof(buffer));
	_tprintf(_T("String1: %s\n"), buffer);

	MemLoader::MemLoadString(handle, 20, buffer, sizeof(buffer));
	_tprintf(_T("String2: %s\n"), buffer);

	MemLoader::MemFreeLibrary(handle);

exit:
	free(data);
}

LPVOID MemLoaderTest::MemoryFailingAlloc(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect, void* userdata)
{
	UNREFERENCED_PARAMETER(address);
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(allocationType);
	UNREFERENCED_PARAMETER(protect);
	UNREFERENCED_PARAMETER(userdata);
	return NULL;
}

LPVOID MemLoaderTest::MemoryMockAlloc(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect, void* userdata)
{
	CallList* calls = (CallList*)userdata;
	CustomAllocFunc current_func = calls->alloc_calls[calls->current_alloc_call++];
	assert(current_func != NULL);
	return current_func(address, size, allocationType, protect, NULL);
}

BOOL MemLoaderTest::MemoryMockFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType, void* userdata)
{
	CallList* calls = (CallList*)userdata;
	CustomFreeFunc current_func = calls->free_calls[calls->current_free_call++];
	assert(current_func != NULL);
	return current_func(lpAddress, dwSize, dwFreeType, NULL);
}

void MemLoaderTest::InitFuncs(void** funcs, va_list args) {
	for (int i = 0; ; i++) {
		assert(i < MAX_CALLS);
		funcs[i] = va_arg(args, void*);
		if (funcs[i] == NULL) break;
	}
}

void MemLoaderTest::InitAllocFuncs(CallList* calls, ...) {
	va_list args;
	va_start(args, calls);
	InitFuncs((void**)calls->alloc_calls, args);
	va_end(args);
	calls->current_alloc_call = 0;
}

void MemLoaderTest::InitFreeFuncs(CallList* calls, ...) {
	va_list args;
	va_start(args, calls);
	InitFuncs((void**)calls->free_calls, args);
	va_end(args);
	calls->current_free_call = 0;
}

void MemLoaderTest:: InitFreeFunc(CallList* calls, CustomFreeFunc freeFunc) {
	for (int i = 0; i < MAX_CALLS; i++) {
		calls->free_calls[i] = freeFunc;
	}
	calls->current_free_call = 0;
}

void MemLoaderTest::TestFailingAllocation(void *data, size_t size) {
	CallList expected_calls;
	HMEMORYMODULE handle;

	InitAllocFuncs(&expected_calls, MemoryFailingAlloc, MemoryFailingAlloc, NULL);
	InitFreeFuncs(&expected_calls, NULL);

	handle = MemoryLoadLibraryEx(
		data, size, MemoryMockAlloc, MemoryMockFree, MemoryDefaultLoadLibrary,
		MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, &expected_calls);

	assert(handle == NULL);
	assert(GetLastError() == ERROR_OUTOFMEMORY);
	assert(expected_calls.current_free_call == 0);

	MemoryFreeLibrary(handle);
	assert(expected_calls.current_free_call == 0);
}

void MemLoaderTest:: TestCleanupAfterFailingAllocation(void *data, size_t size) {
	CallList expected_calls;
	HMEMORYMODULE handle;
	int free_calls_after_loading;

	InitAllocFuncs(&expected_calls,
		MemoryDefaultAlloc,
		MemoryDefaultAlloc,
		MemoryDefaultAlloc,
		MemoryDefaultAlloc,
		MemoryFailingAlloc,
		NULL);
	InitFreeFuncs(&expected_calls, MemoryDefaultFree, NULL);

	handle = MemoryLoadLibraryEx(
		data, size, MemoryMockAlloc, MemoryMockFree, MemoryDefaultLoadLibrary,
		MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, &expected_calls);

	free_calls_after_loading = expected_calls.current_free_call;

	MemoryFreeLibrary(handle);
	assert(expected_calls.current_free_call == free_calls_after_loading);
}

void  MemLoaderTest::TestFreeAfterDefaultAlloc(void *data, size_t size) {
	CallList expected_calls;
	HMEMORYMODULE handle;
	int free_calls_after_loading;

	// Note: free might get called internally multiple times
	InitFreeFunc(&expected_calls, MemoryDefaultFree);

	handle = MemoryLoadLibraryEx(
		data, size, MemoryDefaultAlloc, MemoryMockFree, MemoryDefaultLoadLibrary,
		MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, &expected_calls);

	assert(handle != NULL);
	free_calls_after_loading = expected_calls.current_free_call;

	MemoryFreeLibrary(handle);
	assert(expected_calls.current_free_call == free_calls_after_loading + 1);
}

#ifdef _WIN64

LPVOID MemLoaderTest::MemoryAllocHigh(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect, void* userdata)
{
	int* counter = static_cast<int*>(userdata);
	if (*counter == 0) {
		// Make sure the image gets loaded to an address above 32bit.
		uintptr_t offset = 0x10000000000;
		address = (LPVOID)((uintptr_t)address + offset);
	}
	(*counter)++;
	return MemoryDefaultAlloc(address, size, allocationType, protect, NULL);
}

void MemLoaderTest::TestAllocHighMemory(void *data, size_t size) {
	HMEMORYMODULE handle;
	int counter = 0;
	addNumberProc addNumber;
	HMEMORYRSRC resourceInfo;
	DWORD resourceSize;
	LPVOID resourceData;
	TCHAR buffer[100];

	handle = MemoryLoadLibraryEx(
		data, size, MemoryAllocHigh, MemoryDefaultFree, MemoryDefaultLoadLibrary,
		MemoryDefaultGetProcAddress, MemoryDefaultFreeLibrary, &counter);

	assert(handle != NULL);

	addNumber = (addNumberProc)MemoryGetProcAddress(handle, "addNumbers");
	_tprintf(_T("From memory: %d\n"), addNumber(1, 2));

	resourceInfo = MemoryFindResource(handle, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
	_tprintf(_T("MemoryFindResource returned 0x%p\n"), resourceInfo);

	resourceSize = MemorySizeofResource(handle, resourceInfo);
	resourceData = MemoryLoadResource(handle, resourceInfo);
	_tprintf(_T("Memory resource data: %ld bytes at 0x%p\n"), resourceSize, resourceData);

	MemoryLoadString(handle, 1, buffer, sizeof(buffer));
	_tprintf(_T("String1: %s\n"), buffer);

	MemoryLoadString(handle, 20, buffer, sizeof(buffer));
	_tprintf(_T("String2: %s\n"), buffer);

	MemoryFreeLibrary(handle);
}
#endif  // _WIN64

void MemLoaderTest::TestCustomAllocAndFree(LPCWSTR dllFile)
{
	void *data;
	size_t size;

	data = ReadLibrary(&size, dllFile);
	if (data == NULL)
	{
		return;
	}

	_tprintf(_T("Test MemoryLoadLibraryEx after initially failing allocation function\n"));
	TestFailingAllocation(data, size);
	_tprintf(_T("Test cleanup after MemoryLoadLibraryEx with failing allocation function\n"));
	TestCleanupAfterFailingAllocation(data, size);
	_tprintf(_T("Test custom free function after MemoryLoadLibraryEx\n"));
	TestFreeAfterDefaultAlloc(data, size);
#ifdef _WIN64
	_tprintf(_T("Test allocating in high memory\n"));
	TestAllocHighMemory(data, size);
#endif

	free(data);
}