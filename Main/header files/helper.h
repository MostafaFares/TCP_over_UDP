#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <WS2tcpip.h>
#include <vector>
#include <string>
#include <stdio.h>
#include <iterator>
#include <direct.h>
#include <time.h>
#include <string>
#include <thread>
#include <limits>

#pragma comment (lib, "ws2_32.lib")
#define GetCurrentDir _getcwd

static int MASTERSERVERPORT = 54000;
static const int NUM_CLIENTS = 10;
static const int DATASIZE = 8192;
static const long TIMEOUT = 2; /*seconds*/
static const int SSTHRES = 64; //KB
enum STATE {SLOWSTART, CONGAVOID, FASTRECOV};
using namespace std;

struct packet {
	/*header*/
	uint32_t seqno = 0;
	uint32_t ackno = 0;
	uint16_t cksum = 0;
	bool SYN = false;
	bool ACK = false;
	bool FIN = false;
	uint16_t len = 0;
	/*Data*/
	char data[DATASIZE];
};

struct ack_packet {
	/*header*/
	uint32_t seqno = 0;
	uint32_t ackno = 0;
	uint16_t cksum = 0;
	bool SYN = false;
	bool ACK = false;
	bool FIN = false;
};

class Helper {
public:
	void startWinSock();
	sockaddr_in formAddressInfo(const char* ip, int port);
	SOCKET createNewSocket();
	void ppacket(struct packet p);
	void pAckPacket(struct ack_packet* p);
	int send(SOCKET sock, sockaddr_in& servAddr, packet* packet);
	int sendAck(SOCKET sock, sockaddr_in& servAddr, struct ack_packet* packet);
	int receive(SOCKET sock, sockaddr_in* servAddr, packet* packet, int* len);
	int receiveAck(SOCKET sock, sockaddr_in* servAddr, ack_packet* packet, int* len);
	void putSocketInNonBlockingMode(SOCKET sock);
};

class Timer {
private:
	time_t start, end;
public:
	void startTimer();
	void stopTimer();
	double getElapsedTimeInSec();
};

