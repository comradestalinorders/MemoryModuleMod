#include <WinSock2.h>
#include <winsock.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "stdio.h"


#define DEBUG
void debugg(const char * format, ...);

typedef enum RW {
	READ = 1,
	WRITE = 0
};

typedef struct PIPE_HANDLES {
	HANDLE g_hChildStd_IN_Rd = NULL;
	HANDLE g_hChildStd_IN_Wr = NULL;
	HANDLE g_hChildStd_OUT_Rd = NULL;
	HANDLE g_hChildStd_OUT_Wr = NULL;
};

typedef struct SOCKET_IN_OUT_HANDLE {
	SOCKET s = INVALID_SOCKET;
	HANDLE handle = 0;
	RW rw = READ;
};



int Send(SOCKET s, char * buffer, int buffer_len);
int ConnectServerSendBuffer(char * h_name, char * h_port, char *buffer, int buffer_len);
static SOCKET CreateSocket(char * h_name, char * h_port);
static void CloseSocket(SOCKET s);
static int  Receive(SOCKET s, char * recvbuf, int len);
static int ReadFromSocketWriteToHandle(SOCKET s, HANDLE writeTo);
static int ReadFromStdinWriteSocket(SOCKET s, HANDLE stdInHandle);
static BOOL CreatePipes(PIPE_HANDLES *pHandles, SECURITY_ATTRIBUTES *pSec);
static HANDLE CreateThread(SOCKET_IN_OUT_HANDLE* sh);
static ULONG __stdcall ThreadFunction(LPVOID param);


class SocketOperations {
private :
	SocketOperations();
public :
	static SOCKET  CreateSocket(char * h_name, char * h_port);
	static void CloseSocket(SOCKET s);

	static PROCESS_INFORMATION CreateChildProcess(TCHAR   szCmdline[], PIPE_HANDLES *pHandles);
	static BOOL CreatePipes(PIPE_HANDLES *pHandles, SECURITY_ATTRIBUTES *pSec);
	static HANDLE CreateThread(SOCKET_IN_OUT_HANDLE* sh);

};
