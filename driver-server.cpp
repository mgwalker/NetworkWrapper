#define _NW_VERBOSE
#include "NetWrapper.h"
#include <iostream>
using namespace std;

void Connection(NetClient* nc);
void Disconnection(NetClient* nc);
void ErrorHandler(NetServer* ns, int errCode);
void RecvHandler(NetClient* nc);
void xmain()
{
	int err = 0;
	NetServer ns;
	err = ns.Listen(8422);
	if(err != 0)
		cout << ns.LastError() << endl;
	else
	{
		ns.OnConnect(Connection);
		ns.OnDisconnect(Disconnection);
		ns.OnError(ErrorHandler);
		ns.OnReceiveData(RecvHandler);
		ns.WaitFor("quit");
	}
}

void Connection(NetClient* nc)
{
	nc->Send("Welcome!\r\n",10);
}

void Disconnection(NetClient* nc)
{
}

void ErrorHandler(NetServer* ns, int errCode)
{
	cout << ns->ErrorMessage(errCode) << endl;
}

void RecvHandler(NetClient* nc)
{
	char buff[1025];
	memset(buff, 0, 1025);
	int len = 1024;
	int err = nc->Read(buff, len);
	if(err == 0)
	{
		cout << "Got data (" << len << " bytes)\n  - " << buff << endl;
		nc->Send(buff, len);
	}
	else
		cout << "Data error: " << nc->LastError() << endl;
}