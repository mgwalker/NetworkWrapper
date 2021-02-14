#ifdef WIN32		// For Win32 systems
	#include <winsock.h>
	#include <process.h>
#else				// For everything else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif
#include <stddef.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>

#ifdef _NW_VERBOSE	// For printing in VERBOSE mode
	#include <stdio.h>
#endif

//#ifdef WIN32
//	#define errno WSAGetLastError()
//#endif

//void ListenThread(void* args);

/*
 * NetClient class
 * Generic socket-based client class.
 *
 * METHODS:
 *   NetClient()
 *      - Constructor
 *
 *   ~NetClient()
 *      - Destructor
 *
 *   int Connect(char* ip, int port)
 *      char* ip: IP address to connect to in
 *                dotted form (e.g., "127.0.0.1")
 *      int port: Remote port to connect to
 *      - Attempts to establish a connection with
 *        a server at the specified IP address
 *        listening at the specified port.
 *      RETURNS: 0 on success, error code otherwise
 *
 *   int Send(char* sendData)
 *      char* sendData: Data to send
 *      - Attempts to send specified data to
 *        previously-established connection
 *      RETURNS: 0 on success, error code otherwise
 *
 *   char* ErrorMessage(int errCode)
 *      int errCode: The error code for the message
 *                   to be printed.
 *      - Prints out the error message for the specified
 *        error code.
 *      RETURNS: The error message in a character array
 *
 *   char* LastError()
 *      - Prints out the error message associated with the
 *        most recent error.
 *      RETURNS: The error message in a character array
 */
class NetClient
{
	friend class NetServer;
	public:
		NetClient()
		{
			// Default the most recent error to 0 (none)
			#ifdef _NW_VERBOSE
				printf("(VERBOSE) CLI: Constructor...\n");
			#endif
			_lastError = 0;
		}
		~NetClient()
		{
			// Close the socket
			#ifdef _NW_VERBOSE
				printf("(VERBOSE) CLI: Destructor...\n");
			#endif
			closesocket(_socket);
		}
		int Connect(char* ip, int port)
		{
			// In Win32, start WSA
			#ifdef WIN32
				#ifdef _NW_VERBOSE
					printf("(VERBOSE) CLI: WSA Startup...\n");
				#endif
				if(WSAStartup(MAKEWORD(1,1), &_wsaData) != 0)
				{
					_lastError = 300;
					return 300;	// Win32 Socket Initialization Error
				}
			#endif

			#ifdef _NW_VERBOSE
				printf("(VERBOSE) CLI: Create Socket...\n");
			#endif
			// Attempt to create the client socket
			if((_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
			{
				_lastError = 301;
				return 301; // Invalid Socket Error
			}

			// Clear the socket address struct
			memset(&_iSockAddr, 0, sizeof(_iSockAddr));

			// Set the socket family to internet
			// Set the socket port
			// Set the socket remote IP
			_iSockAddr.sin_family = AF_INET;
			_iSockAddr.sin_port   = htons(port);
			_iSockAddr.sin_addr.s_addr = inet_addr(ip);

			// Attempt to connect to the remote host
			#ifdef _NW_VERBOSE
				printf("(VERBOSE) CLI: Connect...\n");
			#endif
			if(connect(_socket, (SOCKADDR*)&_iSockAddr, sizeof(SOCKADDR)) != 0)
			{
				_lastError = 302;
				return 302; // Socket Connection Error
			}

			// Begin a data-notification thread
			_beginthread(&NetClient::RecvThread, 0, (void*)this);

			// All good, return 0
			return 0;
		}
		int Send(char* sendData, int len)
		{
			// Figure out the message length
			//int mlen = (int)strlen(sendData);

			#ifdef _NW_VERBOSE
			printf("(VERBOSE) CLI: Send data (%i bytes)...\n", len);
			#endif

			// Attempt to send the message
			while(len > 0)
				len = len - send(_socket, sendData, len, 0);
			//if(sent != mlen)

			return 0;
		}
		int Read(char* buffer, int& len)
		{
			len = recv(_socket, buffer, len, 0);
			if(len <= 0)
			{
				if(len == 0)
					return 400;
				else
					return 401;
			}
			else
				return 0;
		}
		int LocalPort()
		{
			// Return the port
			return _iSockAddr.sin_port;
		}
		void WaitFor(char* text)
		{
			//
			//
			// LET THE USER SPECIFY A "WAIT" THREAD
			//
			//
			HANDLE waitThread = (HANDLE)_beginthread(&NetClient::WaitForTextThread, 0, (void*)text);
			WaitForSingleObject(waitThread, INFINITE);
			// Unregister event handlers - shutting down
			OnReceiveDataHandler = NULL;
		}
		void OnReceiveData(void(*fptr)(NetClient*))
		{
			this->OnReceiveDataHandler = fptr;
		}
		char* ErrorMessage(int errCode)
		{
			// Return the appropriate error message
			if(errCode == 300)
				return "CLI300: Win32 Socket Initialization Error\0";
			else if(errCode == 301)
				return "CLI301: Invalid Socket Error\0";
			else if(errCode == 302)
				return "CLI302: Socket Connection Error\0";
			else if(errCode == 400)
				return "CLI400: Read Error (Socket Closed)\0";
			else if(errCode == 401)
				return "CLI401: Read Error\0";
			else
				// If the error code is invalid or not defined,
				// just return a null.
				return "\0";
		}
		char* LastError()
		{
			// Return the error message for the most
			// recent error code
			return ErrorMessage(_lastError);
		}

	private:
		static void RecvThread(void* args)
		{
			((NetClient*)args)->_RecvThread(*(NetClient*)args);
			_endthread();
		}
		void _RecvThread(NetClient nc)
		{
			bool checkForData = true;
			char buff = 0;
			while(checkForData)
			{
				fd_set readSet;
				FD_ZERO(&readSet);
				FD_SET(nc._socket, &readSet);
				select(nc._socket+1, &readSet, NULL, NULL, NULL);
				if(FD_ISSET(nc._socket, &readSet))
				{
					int r = 0;
					if((r = recv(nc._socket, &buff, 1, MSG_PEEK)) <= 0)
					{
						//#ifdef _NW_VERBOSE
						//printf("(VERBOSE) SRV: Client socket receive error...\n");
						//	if(r == 0)
						//		printf("(VERBOSE) SRV: Client socket disconnected...\n");
						//	else
						//		printf("(VERBOSE) SRV: Client socket receive error... %i\n", errno);
						//#endif
						#ifdef _NW_VERBOSE
							printf("(VERBOSE) SRV: Disconnected...\n");
						#endif
						checkForData = false;
					}
					else
						if(this->OnReceiveDataHandler != NULL)
							this->OnReceiveDataHandler(&nc);
				}
			}
		}
		static void WaitForTextThread(void* args)
		{
			const char* haltText = (char*)args;
			int         tLen     = strlen(haltText);
			char*       input    = new char[tLen + 1];
			char*       format   = new char[(tLen / 10) + 4];
			bool        go       = true;

			format[0] = '%';
			for(int i = (tLen / 10); i >= 0 ; i--)
			{
				int powr = pow(10, i);
				format[i + 1] = (char)((tLen/powr) + 48);
			}
			format[(tLen / 10) + 2] = 's';
			format[(tLen / 10) + 3] = 0;

			#ifdef _NW_VERBOSE
				printf("(VERBOSE) SRV: Wait Thread Started - Listening For \"%s\"...\n", haltText);
			#endif

			while(go)
			{
				scanf(format, input);
				go = false;
				for(int i = 0; i < tLen; i++)
					if((input[i] | 0x20) != (haltText[i] | 0x20))
						go = true;
			}

			#ifdef _NW_VERBOSE
				printf("(VERBOSE) SRV: Wait Thread Done...\n");
			#endif
		}
		SOCKET		_socket;	// Socket descriptor
		SOCKADDR_IN	_iSockAddr;	// Socket address struct
		#ifdef WIN32			// WSA data struct, for Win32 sockets
			WSADATA		_wsaData;
		#endif
		int			_lastError;	// Most recent error code
		void(*OnReceiveDataHandler)(NetClient*);
};

/*
 * NetServer class
 * Generic socket-based server class.
 *
 * METHODS:
 *   NetServer()
 *      - Constructor
 *
 *   ~NetServer()
 *      - Destructor
 *
 *   int Listen(int port)
 *      int port: Local port to listen to
 *      - Begins listening on the specified port for
 *        incoming connections.  Spawns a new thread
 *      RETURNS: 0 on success, error code otherwise
 *
 *   char* ErrorMessage(int errCode)
 *      int errCode: The error code for the message
 *                   to be printed.
 *      - Prints out the error message for the specified
 *        error code.
 *      RETURNS: The error message in a character array
 *
 *   char* LastError()
 *      - Prints out the error message associated with the
 *        most recent error.
 *      RETURNS: The error message in a character array
 */
class NetServer
{
	/*private:
		struct ClientAndServer
		{
			NetClient nc;
			void* ns;
		};*/

	public:
		NetServer()
		{
			// Default the most recent error to 0 (none), set
			// the protocol to TCP, and make sure the event
			// handlers are defaulted to unregistered
			#ifdef _NW_VERBOSE
				printf("(VERBOSE) SRV: Constructor...\n");
			#endif
			_lastError = 0;
			_streamProto = SOCK_STREAM;
			OnConnectHandler		= NULL;
			OnDisconnectHandler		= NULL;
			OnReceiveDataHandler	= NULL;
			OnErrorHandler			= NULL;
		}
		~NetServer()
		{
			// Close the socket
			#ifdef _NW_VERBOSE
				printf("(VERBOSE) SRV: Destructor...\n");
			#endif
			closesocket(_sSocket);
		}
		int ListenUDP(int port)
		{
			_streamProto = SOCK_DGRAM;
			return Listen(port);
		}
		int Listen(int port)
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
			if((_sSocket = socket(AF_INET, _streamProto, 0)) == INVALID_SOCKET)
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

			// Start the listener thread
			this->_lThread = (HANDLE)_beginthread(&NetServer::ListenThread, 0, (void*)this);

			// All good, return 0
			return 0;
		}
		void WaitFor(char* text)
		{
			//
			//
			// LET THE USER SPECIFY A "WAIT" THREAD
			//
			//
			HANDLE waitThread = (HANDLE)_beginthread(&NetServer::WaitForTextThread, 0, (void*)text);
			WaitForSingleObject(waitThread, INFINITE);
			// Unregister event handlers - shutting down
			OnConnectHandler = NULL;
			OnDisconnectHandler = NULL;
			OnReceiveDataHandler = NULL;
			OnErrorHandler = NULL;
		}
		void OnConnect(void(*fptr)(NetClient*))
		{
			this->OnConnectHandler = fptr;
		}
		void OnDisconnect(void(*fptr)(NetClient*))
		{
			this->OnDisconnectHandler = fptr;
		}
		void OnReceiveData(void(*fptr)(NetClient*))
		{
			this->OnReceiveDataHandler = fptr;
		}
		void OnError(void(*fptr)(NetServer*, int))
		{
			this->OnErrorHandler = fptr;
		}
		char* ErrorMessage(int errCode)
		{
			// Return the appropriate error message
			if(errCode == 300)
				return "SRV300: Win32 Socket Initialization Error\0";
			else if(errCode == 301)
				return "SRV301: Invalid Socket Error\0";
			else if(errCode == 302)
				return "SRV302: Socket Options Error\0";
			else if(errCode == 303)
				return "SRV303: Socket Bind Error\0";
			else if(errCode == 304)
				return "SRV304: Socket Listen Error\0";
			else if(errCode == 400)
				return "SRV400: Socket Accept-Connection Error\0";
			else
				return 0;
		}

		char* LastError()
		{
			// Return the error message for the most
			// recent error code
			return ErrorMessage(_lastError);
		}

	private:
		// To launch the thread with _beginthread(), the
		// function must be static, so this static method
		// wraps the actual thread function.  Each NetServer
		// object has its own listener thread.
		static void ListenThread(void* args)
		{
			((NetServer*)args)->_ListenThread();
			_endthread();
		}
		void _ListenThread()
		{
			//ListenerThreadArg lta = *(ListenerThreadArg*)args;
			#ifdef _NW_VERBOSE
				printf("(VERBOSE) SRV: Listener Thread Started...\n");
			#endif

			SOCKET      inS;
			SOCKADDR_IN inA;
			memset(&inA, 0, sizeof(SOCKADDR_IN));
			int sze = sizeof(SOCKADDR_IN);
			while((inS = accept(this->_sSocket, (SOCKADDR *)&inA, &sze)) != -1)
			{
				#ifdef _NW_VERBOSE
					printf("(VERBOSE) SRV: Got A Connection...\n");
				#endif
				_recNC._socket		= inS;
				_recNC._lastError	= 0;
				_recNC._iSockAddr	= inA;
				if(OnConnectHandler != NULL)
					OnConnectHandler(&_recNC);
				_clientCopied = false;
				_beginthread(NetServer::ClientThread,0,(void*)this);
				while(!_clientCopied);
			}
			this->_lastError = 400;
			if(OnErrorHandler != NULL)
				OnErrorHandler(this, 400);

			#ifdef _NW_VERBOSE
				printf("(VERBOSE) SRV: Listener Thread Exitting...\n");
			#endif
		}
		static void ClientThread(void* args)
		{
			((NetServer*)args)->_ClientThread();
			_endthread();
		}
		void _ClientThread()
		{
			#ifdef _NW_VERBOSE
				printf("(VERBOSE) SRV: Client Thead Started...\n");
			#endif

			NetClient nc = this->_recNC;
			this->_clientCopied = true;
			bool checkForData = true;
			char buff = 0;
			while(checkForData)
			{
				fd_set readSet;
				FD_ZERO(&readSet);
				FD_SET(nc._socket, &readSet);
				select(nc._socket+1, &readSet, NULL, NULL, NULL);
				if(FD_ISSET(nc._socket, &readSet))
				{
					int r = 0;
					if((r = recv(nc._socket, &buff, 1, MSG_PEEK)) <= 0)
					{
						//#ifdef _NW_VERBOSE
						//printf("(VERBOSE) SRV: Client socket receive error...\n");
						//	if(r == 0)
						//		printf("(VERBOSE) SRV: Client socket disconnected...\n");
						//	else
						//		printf("(VERBOSE) SRV: Client socket receive error... %i\n", errno);
						//#endif
						if(this->OnDisconnectHandler != NULL)
							this->OnDisconnectHandler(&nc);
						#ifdef _NW_VERBOSE
							printf("(VERBOSE) SRV: Client Disconnected...\n");
						#endif
						checkForData = false;
					}
					else
						if(this->OnReceiveDataHandler != NULL)
							this->OnReceiveDataHandler(&nc);
				}
			}

			#ifdef _NW_VERBOSE
				printf("(VERBOSE) SRV: Client Thead Exitting...\n");
			#endif
		}
		static void WaitForTextThread(void* args)
		{
			const char* haltText = (char*)args;
			int         tLen     = strlen(haltText);
			char*       input    = new char[tLen + 1];
			char*       format   = new char[(tLen / 10) + 4];
			bool        go       = true;

			format[0] = '%';
			for(int i = (tLen / 10); i >= 0 ; i--)
			{
				int powr = pow(10, i);
				format[i + 1] = (char)((tLen/powr) + 48);
			}
			format[(tLen / 10) + 2] = 's';
			format[(tLen / 10) + 3] = 0;

			#ifdef _NW_VERBOSE
				printf("(VERBOSE) SRV: Wait Thread Started - Listening For \"%s\"...\n", haltText);
			#endif

			while(go)
			{
				scanf(format, input);
				go = false;
				for(int i = 0; i < tLen; i++)
					if((input[i] | 0x20) != (haltText[i] | 0x20))
						go = true;
			}

			#ifdef _NW_VERBOSE
				printf("(VERBOSE) SRV: Wait Thread Done...\n");
			#endif
		}

		SOCKET				_sSocket;
		SOCKADDR_IN			_iSockAddr;
		#ifdef WIN32
			WSADATA				_wsaData;
		#endif
		HANDLE				_lThread;
		int					_streamProto;
		int					_lastError;
		NetClient			_recNC;
		bool				_clientCopied;
		void(*OnConnectHandler)(NetClient*);
		void(*OnDisconnectHandler)(NetClient*);
		void(*OnReceiveDataHandler)(NetClient*);
		void(*OnErrorHandler)(NetServer*, int);
};