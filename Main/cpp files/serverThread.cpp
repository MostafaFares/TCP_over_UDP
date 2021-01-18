#include "serverThread.h"

void serverThread::run()
{
	cout << "NEW server port:" << myAdrr.sin_port << endl;
	srand(3);
	/*----------------------------------------------wait request---------------------------------------*/
	ack_packet* newAckPacket = new ack_packet;
	packet* reqpacket = new packet;
	newAckPacket->ACK = true;
	int st = h.sendAck(this->mysock, this->clientAddr, newAckPacket);
	if (st == SOCKET_ERROR) {
		wprintf(L"sendto failed with error %d\n", WSAGetLastError());
	}
	int addrSize = sizeof(struct sockaddr_in);

	h.putSocketInNonBlockingMode(this->mysock);

	Timer timer;
	timer.startTimer();

	while (1)
	{
		st = h.receive(this->mysock, &(this->clientAddr), reqpacket, &addrSize);
		if (st == WSAEWOULDBLOCK) {
			/*no-thing to rcv*/
			if (timer.getElapsedTimeInSec() > TIMEOUT) {
				/*if time out, resend Ack*/
				st = h.sendAck(this->mysock, this->clientAddr, newAckPacket);
				if (st == SOCKET_ERROR) {
					wprintf(L"sendto failed with error %d\n", WSAGetLastError());
				}
				else
					timer.startTimer();
			}
		}
		else if (st != SOCKET_ERROR) {
			/*if rcv request*/
			if (newAckPacket->ACK && newAckPacket->ackno == 0) {
				h.ppacket(*reqpacket);
				break;
			}
		}
	}
	/*----------------------------------------------request arrived---------------------------------------*/

	/*Get file size if exist*/
	string fileName = "";
	for (int i = 0; i < reqpacket->len; i++)
		fileName += reqpacket->data[i];
	delete reqpacket;

	/*initialize necessary variables for GBN, counting packets not bytes*/
	vector<packet*> packets;
	vector<double> tracwin;
	int windowSize = 1;
	int slowStartThreshold = (SSTHRES * 1024) / DATASIZE;
	int dupAckCount = 0;
	int lastAckNo = -1;
	int bytesSent = 0;
	int clientSeqNo = 0;
	int packcount = 0;
	int curseqno = 0;
	int windowBase = 0;
	
	FILE* fp;
	std::string filePath = (std::filesystem::current_path() / "2-server" / fileName).u8string();
	//cout << "fileName: " << fileName << endl;
	if (fopen_s(&fp, filePath.c_str(), "rb") != 0) {
		cout << "can not open file serverThread aborted." << endl;
		std::terminate();
	}
	cout << "successfully open the file." << endl << endl;
	ack_packet* ackPacket = new ack_packet;
	int congcount = 0;
	STATE state = SLOWSTART;

	while (1) {

		if (curseqno < windowBase + windowSize &&
			!(feof(fp) && curseqno >= packcount)) {
			
			packet* newPacket;
			/*read from file*/
			char* dataRead = new char[DATASIZE];
			int bytesRead;
			if (!feof(fp)) {
				newPacket = new packet;
				bytesRead = fread(dataRead, 1, DATASIZE, fp);
				memcpy(newPacket->data, dataRead, bytesRead);
				//cout << "data in packet from file" << newPacket->data << endl;
				newPacket->len = bytesRead;
				newPacket->seqno = packcount;
				newPacket->ackno = clientSeqNo;
				if (feof(fp)) {
					newPacket->FIN = true;
				}
				packets.push_back(newPacket);
				packcount++;
			}
			
			/*print packet*/
			/*cout << "send packet:";
			h.ppacket(*packets[curseqno]);
			cout << endl;*/
			
			/*send packet*/
			cout << "PLP:" << PLP << endl;
			if (!dropPacket(PLP)) {
				h.send(this->mysock, this->clientAddr, packets[curseqno]);
				cout << "SEND seqNo:" << packets[curseqno]->seqno << endl;
			}
			else
				cout << "DROP PACKET:" << packets[curseqno]->seqno <<" WITH PLP:" << PLP << "''''''''''''''''''''''''''''''''''''''''''''''''''"<< endl;
				
			/*set timer on base*/
			if (curseqno == windowBase)
				timer.startTimer();
			curseqno++;
			tracwin.push_back(windowSize);
			/*some flow control*/
			//std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		/*receive ack*/
		ZeroMemory(ackPacket, sizeof(struct ack_packet));
		int SenderAddrSize = sizeof(this->clientAddr);
		int status = h.receiveAck(this->mysock, &this->clientAddr, ackPacket, &SenderAddrSize);
		if (status == WSAEWOULDBLOCK) {
			if (timer.getElapsedTimeInSec() > TIMEOUT) {
				/*Time_out*/
				cout << "Time_out base: " << windowBase << endl;
				slowStartThreshold = windowSize / 2;
				windowSize = 1;
				dupAckCount = 0;
				curseqno = windowBase;
				if (state != SLOWSTART)
					state = SLOWSTART;
			}
		}
		else if (status != SOCKET_ERROR) {
			/*recv ack packet*/
			/*cout << "recevied ack packet:" << endl;
			h.pAckPacket(ackPacket);*/
			cout << "RECV achNo:" << ackPacket->ackno << endl << endl;
			std::cout << "curSeqNo=" << curseqno <<" ,base="<<windowBase<<" ,windowSize=" << windowSize << " , " << "SSTHRES=" << slowStartThreshold << endl << endl;
			if (ackPacket->ACK) {
				/*update window*/
				windowBase = ackPacket->ackno + 1;
				clientSeqNo = ackPacket->seqno;
				if (curseqno == windowBase)
					timer.stopTimer();
				else
					timer.startTimer();
				/*check dupAcks*/
				if (lastAckNo == ackPacket->ackno) {
					/*dupAck*/
					if (state == FASTRECOV) {
						windowSize += 1;

						//std::cout << "IN FASTRECOV dup Ack " << "windowSize:" << windowSize << " , " << "SSTHRES:" <<slowStartThreshold << endl;
					}
					else {
						dupAckCount++;
						/*if 3 dupAcks*/
						if (dupAckCount >= 3) {
							slowStartThreshold = windowSize / 2;
							windowSize = slowStartThreshold + 3;
							//std::cout <<"3 Dup Acks " << "windowSize:" << windowSize << " , " << "SSTHRES:" <<slowStartThreshold << endl;
							curseqno = windowBase;
							state = FASTRECOV;
						}
					}
				}
				else {
					/*new Ack*/
					lastAckNo = ackPacket->ackno;
					dupAckCount = 0;
					if (state == SLOWSTART) {
						windowSize++;
						//std::cout << "IN SLOW " << "windowSize:" << windowSize << " , " << "SSTHRES:" <<slowStartThreshold << endl;
					}
					else if (state == CONGAVOID) {
						//windowSize += (1 / windowSize);
						//std::cout << "IN CONGAVOID " << "windowSize:" << windowSize << " , " << "SSTHRES:" <<slowStartThreshold << endl;
						congcount++;
						if (congcount >= windowSize) {
							windowSize++;
							congcount = 0;
						}
					}
					else {
						/*fast recovery*/
						windowSize = slowStartThreshold;
						state = CONGAVOID;

						//std::cout << "FAST to CONGAVOID " << "windowSize:" << windowSize << " , " << "SSTHRES:" <<slowStartThreshold << endl;
					}
				}
				/*flip from slow start to congestion avoidance*/
				if (windowSize >= slowStartThreshold && state == SLOWSTART) {
					//std::cout << "SLOW to CONGAVOID " << "windowSize:"<< windowSize<<" >= "<< "SSTHRES:" << slowStartThreshold << endl;
					state = CONGAVOID;
				}
					
			}
			if (feof(fp) && lastAckNo == packcount - 1) {
				break;
			}
		}
	}
	/*write trace of window size into file*/
	ofstream output;
	string tracfile = "tracwinP1.txt";

	//clear tracwin.txt before write
	output.open(tracfile, ios::out | ios::trunc);
	output.close();

	output.open(tracfile, ios::out | ios::app);
	if (!output.is_open()) {
		cout << "cannot create/open trace file." << endl;
	} else
		for (double p : tracwin) {
			output << (int)(p) << endl;
		}
	

	/*clean memory*/
	cout << "server:"<< id << " port:" << myAdrr.sin_port << " completed its task." << endl;
	fclose(fp);
	output.close();
	for (packet* p: packets) {
		delete p;
	}
	delete ackPacket;
	closesocket(this->mysock);
	delete server;
}

bool serverThread::dropPacket(int PLP)
{
	int r = rand() % 100 + 1;
	if (r <= PLP)
		return true;
	return false;
}
