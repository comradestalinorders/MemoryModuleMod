#include "../incl/SockOperations.h"
#define DEFAULT_BUFLEN    8192

int debugFlag = 0;
#ifdef DEBUG
void debugg(const char * format,  ...) {
	if (!debugFlag) return;
	va_list ap;
	va_start(ap, format);
	vprintf(format, ap);//printf(format, ap);
	va_end(ap);
}
#else
 void debugg(const char * p, ...) { ; }
#endif


/**
 Send buffer over socket ....
*/
int  Send(SOCKET s, char * buffer, int buffer_len) {
	int bytecount;
	if ((bytecount = send(s, buffer, buffer_len, 0)) == SOCKET_ERROR) {
		debugg("%d bytes Send over Socket ..... \n", bytecount);
		debugg("Error sending data %d\n", WSAGetLastError());
		return 0;
	}
	return bytecount;
}


static BOOL CreatePipes(PIPE_HANDLES *pHandles, SECURITY_ATTRIBUTES *pSec) {
	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&(pHandles->g_hChildStd_OUT_Rd), &pHandles->g_hChildStd_OUT_Wr, pSec, 0)) {
		debugg(("StdoutRd CreatePipe failed...."));
		return FALSE;
	}
	

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(pHandles->g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
		debugg(("Stdout SetHandleInformation failed..."));
		return FALSE;
	}
	

	// Create a pipe for the child process's STDIN. 
	if (!CreatePipe(&pHandles->g_hChildStd_IN_Rd, &pHandles->g_hChildStd_IN_Wr, pSec, 0)) {
		debugg(("Stdin CreatePipe failed"));
		return FALSE;
	}
	

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	if (!SetHandleInformation(pHandles->g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
		debugg("Stdin SetHandleInformation failed" );
		return FALSE;
	}
	return TRUE;
}

static PROCESS_INFORMATION CreateChildProcess( TCHAR  * szCmdline, PIPE_HANDLES *pHandles)
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{
	//TCHAR szCmdline[] = TEXT("child");
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = pHandles->g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = pHandles->g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = pHandles->g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process. 
	bSuccess = CreateProcess(
		NULL,
		szCmdline,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

	 // If an error occurs, exit the application. 
	if (!bSuccess);
		//ErrorExit(TEXT("CreateProcess"));
	else
	{
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 
		//CloseHandle(piProcInfo.hProcess);
		//CloseHandle(piProcInfo.hThread);

		// Close handles to the stdin and stdout pipes no longer needed by the child process.
		// If they are not explicitly closed, there is no way to recognize that the child process has ended.
		CloseHandle(pHandles->g_hChildStd_OUT_Wr);
		CloseHandle(pHandles->g_hChildStd_IN_Rd);
	}
	return piProcInfo;
}

static int ReadFromSocketWriteToHandle(SOCKET s, HANDLE stdOutHandle) {
	char recvbuf[DEFAULT_BUFLEN];
	ZeroMemory(recvbuf, DEFAULT_BUFLEN);
	int res = 0;
	DWORD bytesWritten = 0;
	do {
		//read from socket
		debugg("Expecting bytes from socket...\n");
		res = Receive(s, recvbuf, DEFAULT_BUFLEN);
		
		debugg(" %d Bytes Received from Socket .....\n", res);
		if (res <=0) return res;
		
		//write to process stdinHandle
		if (res > 0) {
			if (!WriteFile(stdOutHandle, recvbuf, res, &bytesWritten, NULL)) return -1;
		}
	} while (res > 0);
}

static int ReadFromStdinWriteSocket(SOCKET s, HANDLE stdInHandle) {
	char recvbuf[DEFAULT_BUFLEN];
	ZeroMemory(recvbuf, DEFAULT_BUFLEN);
	int res = 0;
	DWORD bytesRead = 0;
	do {
		//read from  process stdinHandle
		debugg("Expecting bytes from shell...\n");
		res = ReadFile(stdInHandle, recvbuf, DEFAULT_BUFLEN, &bytesRead, NULL);
		debugg("%d Bytes read from shell...\n", bytesRead);
		
		if (!bytesRead) {
			debugg("Returning from  ReadFromStdinWriteSocket .....\n");
			return res;
		}

		//write to socket
		if (res && bytesRead) {
			int ssend = Send(s, recvbuf, bytesRead);
			debugg("%d Bytes send to socket .....\n", ssend);
			if (ssend < 0) return res;
		}
		else {
			return res;
		}
	} while (res > 0);
}


//the thread function that is doing either reading or writing......
static ULONG __stdcall ThreadFunction(LPVOID param) {
	SOCKET_IN_OUT_HANDLE* p = (SOCKET_IN_OUT_HANDLE*)param;
	while (true) {
		if (p->rw) {
			if(ReadFromStdinWriteSocket(p->s, p->handle) <=0) 
			debugg(" ReadFromStdinWriteSocket Thread finished....\n");
			return 0;
		}
		else {
			if (ReadFromSocketWriteToHandle(p->s, p->handle) <= 0)
			debugg(" ReadFromSocketWriteToHandle Thread finished....\n");
			return 0;
		}
	}
	return 0;
}


//creates read or write thread depending on read field...
static HANDLE CreateThread(SOCKET_IN_OUT_HANDLE* sh) {
	return CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		(LPTHREAD_START_ROUTINE)ThreadFunction,       // thread function name
		sh,                     // argument to thread function 
		0,                      // use default creation flags 
		NULL);
}



static int Receive(SOCKET s, char * recvbuf, int len) {
	int iResult=0;
	iResult = recv(s, recvbuf, len, 0);
	if (iResult > 0)
		debugg("Bytes received: %d\n", iResult);
	else if (iResult == 0)
			debugg("Connection closed\n");
	else {
		debugg("recv failed: %d\n", WSAGetLastError());
		return -1;
	}
	return iResult;
}

static SOCKET CreateSocket( char * h_name, char * h_port) {
	char*  host_port = h_port;
	char*  host_name = h_name;

	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	//Initialize socket support WINDOWS ONLY!
	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);

	if (err != 0 || (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2)) {
		debugg("Could not find useable sock dll %d\n", WSAGetLastError());
		return 0;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	err = getaddrinfo(host_name, host_port, &hints, &result);
	if (err != 0) {
		debugg("getaddrinfo failed: %d\n", err);
		WSACleanup();
		return INVALID_SOCKET;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			debugg("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return INVALID_SOCKET;
		}

		// Connect to server.
		err = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (err == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		debugg("Unable to connect to server!\n");
		WSACleanup();
		return INVALID_SOCKET;
	}
	return ConnectSocket;
}


static void CloseSocket(SOCKET s) {
	closesocket(s);
}

SOCKET SocketOperations::CreateSocket( char * h_name, char * h_port) {
	return ::CreateSocket(h_name, h_port);
}

void SocketOperations::CloseSocket(SOCKET s) {
	::CloseSocket(s);
}

BOOL SocketOperations::CreatePipes(PIPE_HANDLES *pHandles, SECURITY_ATTRIBUTES *pSec) {
	return ::CreatePipes(pHandles, pSec);
}

HANDLE  SocketOperations::CreateThread(SOCKET_IN_OUT_HANDLE * sh) {
	return::CreateThread(sh);
}
PROCESS_INFORMATION SocketOperations::CreateChildProcess(TCHAR szCmdline[], PIPE_HANDLES *pHandles) {
	return ::CreateChildProcess(szCmdline, pHandles);
}

/*
int ReceiveAll(SOCKET s, char **lpOut) {
	int res = 0;
	int resSum = 0;
	char recvbuf[DEFAULT_BUFLEN];
	char *pAllocAll = NULL;
	do {
		//WriteFile()
		char *pAlloc1 = NULL;
		res = Receive(s, recvbuf, DEFAULT_BUFLEN);
		pAlloc1 = (char*)malloc(resSum + res);
		if(pAllocAll != NULL) memcpy(pAlloc1, pAllocAll, resSum);
		//add the newly read buffer
		memcpy(&(pAlloc1[resSum]), recvbuf, res);
		//free memory allocated so far
		if (pAllocAll != NULL) free(pAllocAll);
		pAllocAll = pAlloc1;
		resSum += res;
	} while (res > 0);
	*lpOut = pAllocAll;
	return resSum;
}

int ConnectServerSendBuffer(char * h_name, char * h_port, char *buffer, int buffer_len) {
	int bytecount;
	SOCKET ConnectSocket = INVALID_SOCKET;

	//cereate the socket to connect to.....
	if (!CreateSocket(&ConnectSocket, h_name, h_port)) return 0;


	if ((bytecount = send(ConnectSocket, buffer, buffer_len, 0)) == SOCKET_ERROR) {
		//fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		//goto FINISH;
		return 0;
	}
	//printf("Sent bytes %d\n", bytecount);
	closesocket(ConnectSocket);
	return 1;
}*/


int ConnectServerSendBuffer(char * h_name, char * h_port, char *buffer, int buffer_len) {

	char*  host_port = h_port;
	char*  host_name = h_name;

	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	//Initialize socket support WINDOWS ONLY!
	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0 || (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 2)) {
		//fprintf(stderr, "Could not find useable sock dll %d\n", WSAGetLastError());
		return 0;
	}


	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	err = getaddrinfo(host_name, host_port, &hints, &result);
	if (err != 0) {
		//printf("getaddrinfo failed: %d\n", err);
		WSACleanup();
		return 0;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			//printf("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 0;
		}

		// Connect to server.
		err = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (err == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		//printf("Unable to connect to server!\n");
		WSACleanup();
		return 0;
	}
	int bytecount;

	if ((bytecount = send(ConnectSocket, buffer, buffer_len, 0)) == SOCKET_ERROR) {
		//fprintf(stderr, "Error sending data %d\n", WSAGetLastError());
		//goto FINISH;
		return 0;
	}
	//printf("Sent bytes %d\n", bytecount);

	closesocket(ConnectSocket);
	return 1;
}