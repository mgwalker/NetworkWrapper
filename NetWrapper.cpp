#include "NetWrapper.h"
#ifdef WIN32		// For Win32 systems
	#include <winsock.h>
	#include <process.h>
	#include <stddef.h>
	#include <stdlib.h>
	#include <conio.h>
#else				// For everything else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#ifdef _NW_VERBOSE	// For printing in VERBOSE mode
#include <stdio.h>
#endif

int NetServer::Listen(int port)
{
	char yes = 1;
	// On Win32, start WSA
	#ifdef WIN32
		#ifdef _NW_VERBOSE
			printf("(VERBOSE) SRV: WSA Startup...\n");
		#endif
		if(WSAStartup(MAKEWORD(1,1), &_wsaData) != 0)
		{
			_lastError = 300;
			return 300;	// Win32 Socket Initialization Error
		}
	#endif

	// Create the listener socket
	#ifdef _NW_VERBOSE
		printf("(VERBOSE) SRV: Create Socket...\n");
	#endif
    if((_sSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		_lastError = 301;
		return 301;
	}

	// Set the socket options
	#ifdef _NW_VERBOSE
		printf("(VERBOSE) SRV: Set Socket Options...\n");
	#endif
	if(setsockopt(_sSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(char)) == -1)
	{
		_lastError = 302;
		return 302;
	}

	// Clear the socket address structure
	memset(&_iSockAddr, 0, sizeof(_iSockAddr));

	// Set the socket family to internet
	// Set the socket to listen on the port specified
	// Bind to whatever address is available
	_iSockAddr.sin_family = AF_INET;
	_iSockAddr.sin_port   = htons(port);
	_iSockAddr.sin_addr.s_addr = INADDR_ANY;

	// Bind the socket
	#ifdef _NW_VERBOSE
		printf("(VERBOSE) SRV: Bind Socket...\n");
	#endif
	if(bind(_sSocket, (SOCKADDR*)&_iSockAddr, sizeof(SOCKADDR)) == -1)
	{
		_lastError = 303;
		return 303;
	}

	// Begin listening
	#ifdef _NW_VERBOSE
		printf("(VERBOSE) SRV: Socket listen...\n");
	#endif
	if(listen(_sSocket, 2) == -1)
	{
		_lastError = 304;
		return 304;
	}

	_threadRunning = true;
	ListenerThreadArg lta;
	lta._ns = *this;
	lta._sSocket = _sSocket;
	//_lThread = (HANDLE)_beginthreadex(NULL, 0, &ListenThread, this, 0, NULL);
	_lThread = (HANDLE)_beginthread(ListenThread, 0, &lta);

	// All good, return 0
	return 0;
}