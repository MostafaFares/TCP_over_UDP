#pragma once
#include "helper.h"

class serverThread
{
private:
	int id;
	int PLP;
	Helper h;
	thread* server;
	sockaddr_in clientAddr;
	sockaddr_in myAdrr;
	SOCKET mysock;

public:

	serverThread(int id,SOCKET newSock, sockaddr_in clientAddress, sockaddr_in myAddress, int plp) {
		this->id = id;
		myAdrr = myAddress;
		mysock = newSock;
		clientAddr = clientAddress;
		server = new thread([this]() {run(); });
		server->detach();
		this->PLP = plp;
	}
	void run();
	bool dropPacket(int PLP);
};