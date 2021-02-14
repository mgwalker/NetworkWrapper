#define _NW_VERBOSE
#include "NetWrapper.h"
#include <math.h>
#include <iostream>
using namespace std;

NetClient nc;

void ReceiveHandler(NetClient* nc);
void SendLoop(void* end);
void main()
{
	int err = 0;
	nc.OnReceiveData(ReceiveHandler);
	err = nc.Connect("127.0.0.1", 8422);
	if(err != 0)
		cout << nc.LastError() << endl;
	else
	{
		char buff[1025];
		memset(buff, 0, 1025);
		int len = 1024;
		nc.Read(buff, len);
		cout << buff << endl;
		nc.Send("Hello world",11);
		char quit[5] = "QUIT";
		HANDLE hnd = (HANDLE)_beginthread(SendLoop, 0, quit);
		WaitForSingleObject(hnd,INFINITE);
	}
}

void ReceiveHandler(NetClient* nc)
{
	char buff[1025];
	memset(buff, 0, 1024);
	int len = 1024;
	nc->Read(buff, len);
	cout << "FROM SERVER: " << buff << endl;
}

void SendLoop(void* _end)
{
	char*		end		 = (char*)_end;
	int         tLen     = strlen(end);
	char*       input    = new char[1025];
	bool        go       = true;
	memset(input, 0, 1025);

	while(go)
	{
		scanf("%1024s", input);
		go = false;
		for(int i = 0; i < strlen(input); i++)
			if((input[i] | 0x20) != (end[i] | 0x20))
				go = true;
		if(go)
			nc.Send(input, strlen(input));
        memset(input, 0, tLen + 1);
	}
	_endthread();
}