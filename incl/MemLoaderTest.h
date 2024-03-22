#pragma once
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <malloc.h>
#pragma warning(disable : 4996)
#include "../incl/MemLoader.hpp"

#define MAX_CALLS 20
typedef int(*addNumberProc)(int, int);
typedef int(*myfun)();



struct CallList {
	int current_alloc_call, current_free_call;
	CustomAllocFunc alloc_calls[MAX_CALLS];
	CustomFreeFunc free_calls[MAX_CALLS];
};



class MemLoaderTest{

private :
	MemLoaderTest();
public:
	static void LoadFromFile(LPCWSTR dllFile);
	static void* ReadLibrary(size_t* pSize, LPCWSTR dllFile);
	static void LoadFromMemory(LPCWCHAR dllFile);
	static LPVOID MemoryFailingAlloc(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect, void* userdata);
	static LPVOID MemoryMockAlloc(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect, void* userdata);
	static BOOL MemoryMockFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType, void* userdata);
	static void InitFuncs(void** funcs, va_list args);
	static void InitAllocFuncs(CallList* calls, ...);
	static void InitFreeFuncs(CallList* calls, ...);
	static void InitFreeFunc(CallList* calls, CustomFreeFunc freeFunc);
	static void TestFailingAllocation(void *data, size_t size);
	static void TestCleanupAfterFailingAllocation(void *data, size_t size);
	static void TestFreeAfterDefaultAlloc(void *data, size_t size);
	#ifdef _WIN64
	static LPVOID MemoryAllocHigh(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect, void* userdata);
	static void TestAllocHighMemory(void *data, size_t size);
	#endif
	static void TestCustomAllocAndFree(LPCWSTR dllFile);
};

