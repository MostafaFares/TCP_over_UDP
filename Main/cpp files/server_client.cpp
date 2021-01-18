#include "server_client.h"

void Client::run() {
	/*set up connection*/
	serverAddr = h.formAddressInfo("127.0.0.1", MASTERSERVERPORT);
	mysock = h.createNewSocket();

	/*get filename from cmd*/
	string fileName;
	getline(cin, fileName);

	/*prepare request packet*/
	packet* reqpacket = new packet;
	const char* fname = fileName.c_str();
	memcpy(reqpacket->data, fname, fileName.size());
	reqpacket->len = (uint16_t)fileName.size();

	/*two-way handshake*/
	handShaking();
	
	/*send request packet*/
	int bytesSent = h.send(this->mysock, this->serverAddr, reqpacket);
	if (bytesSent == SOCKET_ERROR) {
		wprintf(L"send request failed with error %d\n", WSAGetLastError());
		exit(-1);
	}
	std::cout << "successfully sent to server: " << endl;
	h.ppacket(*reqpacket);
	
	/*Handle rcv packets*/
	int serverAddrlen = sizeof(sockaddr_in);
	sockaddr_in servAddr;
	int lastrcv = -1;
	int mySeqNo = 0;
	packet* rcvpacket = new packet;
	ack_packet* newAckPacket = new ack_packet;
	
	std::string filePath = (std::filesystem::current_path() / "1-client" / fileName).u8string();
	std::filesystem::remove(filePath);
	ofstream output;

	while (1) {
		/*rcv packet*/
		int status = h.receive(this->mysock, &servAddr, rcvpacket, &serverAddrlen);
		//cout << "serverAddr rcv packets:" << this->serverAddr.sin_port << endl;
		/*if there is a packet on socket rcv*/
		if (status != WSAEWOULDBLOCK && status != SOCKET_ERROR) {
			/*deal with packet*/
			bool in_order = false;
			if (rcvpacket->seqno == lastrcv + 1) {
				/*cout << "packet rcv in order:" << endl;
				h.ppacket(*rcvpacket);*/
				/*append packet to a file */
				output.open(filePath, ios::out | ios::binary | ios::app);
				if (!output.is_open()) {
					std::cout << "cannot create/open file." << endl;
					exit(-1);
				}
				output.write(rcvpacket->data, rcvpacket->len);
				output.close();
				lastrcv++;
				in_order = true;
			}
			/*send ack*/
			newAckPacket->ACK = true;
			if (in_order) {
				newAckPacket->ackno = rcvpacket->seqno;
				cout << "IN ORDER RECV, seqNo:" << rcvpacket->seqno << endl;
			}
			else {
				cout << "OUT OF ORDER RECV, seqNo:" << rcvpacket->seqno << endl;
				newAckPacket->ackno = lastrcv;
			}
			//in_order ? newAckPacket->ackno = rcvpacket->seqno : newAckPacket->ackno = lastrcv;
			/*cout << "server port to send ack to: " << serverAddr.sin_port << endl;
			h.pAckPacket(newAckPacket);*/
			status = h.sendAck(this->mysock, this->serverAddr, newAckPacket);
			if (status == SOCKET_ERROR) {
				exit(-1);
			}
			if (status > 0 && rcvpacket->FIN && in_order)
				break;
			ZeroMemory(rcvpacket, sizeof(struct packet));
			ZeroMemory(newAckPacket, sizeof(struct ack_packet));
		}
	}
	delete rcvpacket;
	delete newAckPacket;
	// Close the socket
	closesocket(mysock);

	// Close down Winsock
	WSACleanup();
}

void Client::handShaking()
{
	/*Two way handshake*/
	ack_packet* newAckPacket = new ack_packet;
	ack_packet* newSynPacket = new ack_packet;
	newSynPacket->SYN = true;

	int bytesSent = h.sendAck(this->mysock, this->serverAddr, newSynPacket);
	if (bytesSent == SOCKET_ERROR) {
		wprintf(L"sendto failed with error %d\n", WSAGetLastError());
	}

	h.putSocketInNonBlockingMode(this->mysock);
	int serverAddrlen = sizeof(sockaddr_in);
	Timer timer;
	timer.startTimer();

	while (1)
	{	
		ZeroMemory(&this->serverAddr, serverAddrlen);
		int status = h.receiveAck(this->mysock, &(this->serverAddr), newAckPacket, &serverAddrlen);
		/*cout << "server port:" << this->serverAddr.sin_port << endl;
		cout << "server addr:" << this->serverAddr.sin_addr.S_un.S_addr << endl;*/
		if (status == WSAEWOULDBLOCK) {
			/*no-thing to rcv*/
			if (timer.getElapsedTimeInSec() > TIMEOUT) {
				/*if time out, resend SYN*/
				bytesSent = h.sendAck(this->mysock, this->serverAddr, newSynPacket);
				if (bytesSent != SOCKET_ERROR)
					timer.startTimer(); 	
			}
		}
		else if (status != SOCKET_ERROR) {
			/*if rcv something*/
			h.pAckPacket(newAckPacket);
			if (newAckPacket->ACK && newAckPacket->ackno == 0) {
				std::cout << "Connected to server_port: " << this->serverAddr.sin_port << endl;
				break;
			}
		}
	}
}


void Server::run() {
	SOCKET listenSock = h.createNewSocket();
	sockaddr_in serverAddress = h.formAddressInfo("127.0.0.1", MASTERSERVERPORT);
	ack_packet* newAckPacket = new ack_packet;
	sockaddr_in client;
	int clientLength = sizeof(client);
	int countActiveClients = 0; //num_of_live_clients
	if (bind(listenSock, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		std::cout << "Can't bind socket! " << WSAGetLastError() << endl;
		exit(1);
	}
	//cout << "Master server Successfully bind on port:" << serverAddress.sin_port << endl;
	cout << "Enter PLP: ";
	string p;
	std::getline(cin, p);
	int prob = stoi(p);

	while (true)
	{
		/* rcv SYN packet */
		ZeroMemory(&client, clientLength);
		int status = h.receiveAck(listenSock, &client, newAckPacket, &clientLength);
		if (status == SOCKET_ERROR) {
			wprintf(L"Master server recvfrom failed with error: %d\n", WSAGetLastError());
		}
		else {
			h.pAckPacket(newAckPacket);
			if (!newAckPacket->SYN)
				continue;
			//create a new socket for each client to communicate on.
			SOCKET newsock = h.createNewSocket(); 
			sockaddr_in newclient = client;
			countActiveClients++;
			//check num_clients //TODO
			countActiveClients %= NUM_CLIENTS;
			//same server_addr with new port number to the new serverThread.
			sockaddr_in newServerAddress = h.formAddressInfo("127.0.0.1", MASTERSERVERPORT + countActiveClients);
			if (bind(newsock, (sockaddr*)&newServerAddress, sizeof(newServerAddress)) == SOCKET_ERROR)
			{
				/*bind failed -> reset*/
				countActiveClients--;
				closesocket(newsock);
				std::cout << "Can't bind a new socket for client!..socket closed back.\n" << WSAGetLastError() << endl;
				continue;
			}
			std::cout << "NEW_CLIENT Arrived id="<< countActiveClients <<" : SYN NEW_CLIENT----NEW_CLIENT---NEW_CLIENT----NEW_CLIENT:" << endl;
			/*cout << "client port:" << newclient.sin_port << endl;
			cout << "client addr:" << newclient.sin_addr.S_un.S_addr<< endl;*/
			serverThread* newThread = new serverThread(countActiveClients, newsock, newclient, newServerAddress, prob);
			//clients[countActiveClients] = true;
		}
	}

	// Close mysocket on time out
	//closesocket(mysock);

	// Shutdown winsock
	WSACleanup();

}