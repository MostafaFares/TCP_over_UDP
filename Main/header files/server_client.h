#pragma once
#include "serverThread.h"

#define NUM_THREADS 20

class Client
{
private:
	Helper h;

	time_t timeSinceIdle;

	sockaddr_in serverAddr;

	sockaddr_in myAdrr;

	SOCKET mysock;

public:
	void run();
	void handShaking();
};



class Server
{
private:
	Helper h;
	bool clients[NUM_CLIENTS];
public:
	void run();
};