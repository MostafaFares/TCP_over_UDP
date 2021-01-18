#include "helper.h"

void Helper::startWinSock()
{
	WSADATA data;
	WORD version = MAKEWORD(2, 2);
	int wsOk = WSAStartup(version, &data);
	if (wsOk != 0)
	{
		cout << "Can't start Winsock! " << wsOk;
	}
}

sockaddr_in Helper::formAddressInfo(const char* ip, int port)
{
	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = port;
	inet_pton(AF_INET, ip, &address.sin_addr);
	return address;
}

SOCKET Helper::createNewSocket()
{
	SOCKET mysock = socket(AF_INET, SOCK_DGRAM, 0);

	if (mysock == SOCKET_ERROR) {
		perror("Error create a socket.");
		exit(1);
	}
	return mysock;
}

void Helper::ppacket(packet p)
{
	cout << endl;
	cout << "PACKET: " << endl;
	cout << "seqno->" << p.seqno << " , ackno->" << p.ackno << endl;
	cout << "isACK:" << p.ACK << " , isSYN:" << p.SYN << " , isFIN:" << p.FIN << endl;
	cout << "dataLen:" << p.len << endl;
	cout << "Data:" << endl;
	char* data = p.data;
	for (int i = 0; i < p.len; i++) {
		cout << data[i];
	}
	cout << endl;
	cout << endl;
}

void Helper::pAckPacket(ack_packet* p)
{
	cout << endl;
	cout << "ack_packet:" << endl;
	cout << "seqno->" << p->seqno << " ,ackno->" << p->ackno << endl;
	cout << "syn->" << p->SYN << "ack->" << p->ACK << endl; cout << endl;
}

int Helper::send(SOCKET sock, sockaddr_in &servAddr, packet* pack)
{
	int bytesSent = sendto(sock, reinterpret_cast<char*>(pack), sizeof(packet), 0, (SOCKADDR*)&servAddr, sizeof(servAddr));
	if (bytesSent == SOCKET_ERROR) {
		wprintf(L"sendto failed with error %d\n", WSAGetLastError());
		return 0;
	}
	return bytesSent;
}

int Helper::sendAck(SOCKET sock, sockaddr_in& servAddr, ack_packet* packet)
{
	int bytesSent = sendto(sock, reinterpret_cast<char*>(packet), sizeof(ack_packet), 0, (SOCKADDR*)&servAddr, sizeof(servAddr));
	if (bytesSent == SOCKET_ERROR) {
		wprintf(L"sendto failed with error %d\n", WSAGetLastError());
		return SOCKET_ERROR;
	}
	return bytesSent;
}


int Helper::receive(SOCKET sock, sockaddr_in* servAddr, packet* packet, int* len)
{
	
	int bytesReceived = recvfrom(sock, reinterpret_cast<char*>(packet), sizeof(struct packet), 0, (SOCKADDR*)servAddr, len);
	if (bytesReceived == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEWOULDBLOCK) {
			/*no-thing to recv*/
			return WSAEWOULDBLOCK;
		}
		wprintf(L"rcv failed with error %d\n", WSAGetLastError());
		return SOCKET_ERROR;
	}
	return bytesReceived;
}

int Helper::receiveAck(SOCKET sock, sockaddr_in* servAddr, ack_packet* packet, int* len)
{
	int bytesReceived = recvfrom(sock, reinterpret_cast<char*>(packet), sizeof(struct ack_packet), 0, (SOCKADDR*)servAddr, len);
	if (bytesReceived == SOCKET_ERROR) {
		if (WSAGetLastError() == WSAEWOULDBLOCK) {
			/*no-thing to recv*/
			return WSAEWOULDBLOCK;
		}
		wprintf(L"rcv failed with error %d\n", WSAGetLastError());
		return SOCKET_ERROR;
	}
	return bytesReceived;
}

void Helper::putSocketInNonBlockingMode(SOCKET sock)
{
	unsigned long ul = 1;
	while (ioctlsocket(sock, FIONBIO, (unsigned long*)&ul) == SOCKET_ERROR)
		cout << "Failed to put the socket into non-blocking mode." << endl;
}

void Timer::startTimer()
{
	time(&this->start);
}

void Timer::stopTimer()
{
	this->start = 0;
	this->end = 0;
}

double Timer::getElapsedTimeInSec()
{
	time(&this->end);
	return double(this->end - this->start);
}
