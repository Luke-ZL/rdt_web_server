//
//  client.cpp
//  
//
//  Created by Lihang Liu on 6/6/19.
//

#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <vector>
#include <fstream>
#include <sys/types.h>
#include <algorithm>
#include <netdb.h>
#include <sys/socket.h>

#include "packet.h"

using namespace std;

packet init[2];
uint32_t SeqNum_SERVER;
uint32_t SeqNum_CLIENT;


vector<packet> Sender_window;
//int ackNumber = 0; //sending packet's ack number, = receiving ack packet's seq number + 1
//int seqNumber = 0; //sending packet's sequence number


int cwnd = 512;
int ssthresh = 5120;

void printMessage(packet mPacket, bool Sent, bool isDup){
  if ((mPacket.getSeqNum() == 0) && (mPacket.getAckNum() == 0)) return;
    if(Sent)
        cout << "SEND " << mPacket.getSeqNum() << " " << mPacket.getAckNum() << " " << cwnd << " " << ssthresh;
    else
        cout << "RECV " << mPacket.getSeqNum() << " " << mPacket.getAckNum() << " " << cwnd << " " << ssthresh;
    
    if(mPacket.isACK())
        cout << " " << "ACK";
    if(mPacket.isSYN())
        cout << " " << "SYN";
    if(mPacket.isFIN())
        cout << " " << "FIN";
    if(Sent and isDup)
        cout << " " << "DUP";
    cout << endl;
}

void clientOpenConnection(int sockfd, struct sockaddr_in &addr){
    
    //send out SYN packet
    char send[PACKET_SIZE];
    srand(time(NULL));

    packet sendingPacket;
    SeqNum_CLIENT = rand() % 25601;
    sendingPacket.setFlag(SYN_FLAG);
    sendingPacket.setAckNum(0);
    sendingPacket.setSeqNum(SeqNum_CLIENT);
    sendingPacket.DeConstructPacket(send);
    printMessage(sendingPacket, true, false);

    //cout << (int)send[0] << ' ' << (int)send[1] << ' '<< (int)send[2] << ' '<< (int)send[3] << ' ';
    
    if(sendto(sockfd, send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        cerr << "ERROR: sending packet failed" << endl;
        exit(1);
    }
    SeqNum_CLIENT = (SeqNum_CLIENT + 1) % 25601;                
    sendingPacket.initTimer();
    sendingPacket.setSent();
    
    socklen_t len = sizeof(struct sockaddr_in);
    //receive ACK+SYN packet from server
    char buf[PACKET_SIZE+1];

    while(true){
        int ret = recvfrom(sockfd, buf, PACKET_SIZE, 0, (struct sockaddr*)&addr, &len);
        if(ret>0){
            buf[ret] = 0;
            packet receivedPacket;
            receivedPacket.ConstructPacket(buf);
            printMessage(receivedPacket,false,false);
            if(receivedPacket.isACK() and receivedPacket.isSYN() and receivedPacket.getAckNum() == SeqNum_CLIENT){
                SeqNum_CLIENT = receivedPacket.getAckNum();
               	SeqNum_SERVER = receivedPacket.getSeqNum();
                return;
            }
        }
    }
}


void sendFile(int sockfd, struct sockaddr_in &addr, long long int fileSize, char* fileBuffer)
{
  //int counter = 0;
  //cout << fileSize << endl;
    int packet_count = 0;
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int lastACK = -1;
    int dupACK = -1;
    int dupACK_count = 0;
    bool FR = false;
    bool CA = false;
    char send_buf[PACKET_SIZE + 1];
    char read_buf[PACKET_SIZE + 1];
    while ((fileSize > 0) || (Sender_window.size() > 0)) {
      while ((Sender_window.size() < cwnd / PAYLOAD) && (fileSize > 0) )
        {
            packet sending_packet;
            sending_packet.setSeqNum(SeqNum_CLIENT);
            sending_packet.setAckNum((SeqNum_SERVER + 1) % 25601);
            if (fileSize < PAYLOAD) {
                sending_packet.setPayload(fileBuffer + packet_count * PAYLOAD, (int)fileSize);
                fileSize = 0;
		//cout << fileSize << endl;
            }
            else {
                sending_packet.setPayload(fileBuffer + packet_count * PAYLOAD, (int)PAYLOAD);
                fileSize -= PAYLOAD;
		//cout << fileSize << endl;
            }
            sending_packet.DeConstructPacket(send_buf);

            if (sendto(sockfd, send_buf, PACKET_SIZE, 0, (struct sockaddr *)&addr,
                sizeof(addr)) < 0) {
                cerr << "Could not send to the client" << endl;
            }
            else {
                Sender_window.push_back(sending_packet);
                Sender_window.back().setSent();
                Sender_window.back().initTimer();
                cout << "SEND " << SeqNum_CLIENT << ' ' << sending_packet.getAckNum() << ' ' << cwnd << ' ' << ssthresh << endl;
                SeqNum_CLIENT = (SeqNum_CLIENT + Sender_window.back().getLength()) % 25601;
                packet_count++;
		//cout << fileSize << endl;
            }
        }// end while

        memset(read_buf, 0, PACKET_SIZE + 1);
        int recvlen = recvfrom(sockfd, read_buf, PACKET_SIZE, 0 | MSG_DONTWAIT,
            (struct sockaddr *)&addr, &sin_size);
        if (recvlen > 0) {
            packet receiving_packet;
            read_buf[recvlen] = 0;
            receiving_packet.ConstructPacket(read_buf);
            SeqNum_SERVER = receiving_packet.getSeqNum();
            int Server_ack = receiving_packet.getAckNum();
            cout << "RECV " << SeqNum_SERVER << ' ' << Server_ack << ' ' << cwnd << ' ' << ssthresh << " ACK";
            if (Server_ack == lastACK) {
	      cout << " DUP" << endl;
                dupACK_count++;
                if (dupACK == 3) {
                    FR = true;
                    ssthresh = max(cwnd / 2, 1024);
                    cwnd = ssthresh + 1536;
                    Sender_window[0].DeConstructPacket(send_buf);
                    if (sendto(sockfd, send_buf, PACKET_SIZE, 0, (struct sockaddr *)&addr,
                        sizeof(addr)) < 0) {
                        cerr << "Could not send to the client" << endl;
                    }
                    else {
                        Sender_window[0].initTimer();
                        cout << "SEND " << Sender_window[0].getSeqNum() << ' ' << (receiving_packet.getSeqNum() + 1) % 25601 << ' ' << cwnd << ' ' << ssthresh << ' ' << "DUP" << endl;
                    }
                }
                else if (dupACK > 3) {
                    cwnd += PAYLOAD;
                    if (cwnd > 10240) cwnd = 10240;
                }
                else continue;
            }
            else {
	        cout << endl;
                lastACK = Server_ack;
                dupACK_count = 0;

                int move_count = 0;
                for (int i = 0; i < Sender_window.size(); i++) {
		  int Window_ack = (Sender_window[i].getSeqNum()+1) % 25601;
		  if (Window_ack < Server_ack) move_count++;
		  else if (Server_ack < Window_ack) {
		    if ((Window_ack - Server_ack) > 12800) move_count++;
		    else break;
		  }
                }
		
                Sender_window.erase(Sender_window.begin(), Sender_window.begin() + move_count);
		//cout <<  Sender_window.size() << endl;
                if (FR) {
                    FR = false;
                    CA = true;
                    cwnd = ssthresh;
                    continue;
                }
                if (cwnd >= ssthresh) CA = true;
                if (CA) {
		  cwnd += PAYLOAD * PAYLOAD / cwnd;
		  if (cwnd > 10240) cwnd = 10240;
		}
                else {
		  cwnd += PAYLOAD;
		  if (cwnd > 10240) cwnd = 10240;
		}
            }
        } //end if
        if (Sender_window.size() > 0) {
            if (Sender_window[0].checkTimeout()) {
                FR = true;
                ssthresh = max(cwnd / 2, 1024);
                cwnd = ssthresh + 1536;
                Sender_window[0].DeConstructPacket(send_buf);
                if (sendto(sockfd, send_buf, PACKET_SIZE, 0, (struct sockaddr *)&addr,
                    sizeof(addr)) < 0) {
                    cerr << "Could not send to the client" << endl;
                }
                else {
                    Sender_window[0].initTimer();
                    cout << "SEND " << Sender_window[0].getSeqNum() << ' ' << Sender_window[0].getAckNum() << ' ' << cwnd << ' ' << ssthresh << endl;
                }
            }
        }
	
	/*for(int i = 0;i < Sender_window.size();i++)
	  cout << Sender_window[i].getSeqNum() << ' ';
	  cout << endl;*/
	//cout << Sender_window.size() << ' ' << Sender_window[0].getSeqNum() <<endl;
    } //end while
    
}


void clientCloseConnection(int sockfd, struct sockaddr_in &addr){
    packet sendFINPacket;
    //SeqNum_CLIENT = (SeqNum_CLIENT + 1) % 25601;
    packet receivedPacket;
    char buffer[PACKET_SIZE+1];
    char send[PACKET_SIZE+1];
    
    
    //send FIN packet
    sendFINPacket.setSeqNum(SeqNum_CLIENT);
    sendFINPacket.setAckNum(0);
    sendFINPacket.setFlag(FIN_FLAG);
    sendFINPacket.DeConstructPacket(send);
    
    printMessage(sendFINPacket,true,false);

    
    if(sendto(sockfd, send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        cerr << "ERROR: sending packet failed" << endl;
        exit(1);
    }
	SeqNum_CLIENT = (SeqNum_CLIENT + 1) % 25601;
    sendFINPacket.setSent();
    sendFINPacket.initTimer();
    memset((char*)&send,0,PACKET_SIZE+1);
    
    socklen_t len = sizeof(struct sockaddr_in);
    chrono::time_point<chrono::system_clock> timer;
    chrono::time_point<chrono::system_clock> now;
    //sendFINPacket.initTimer();
    while (true){
     ssize_t ret = recvfrom(sockfd, buffer, PACKET_SIZE, 0 | MSG_DONTWAIT, (struct sockaddr*)&addr, &len);
     if (ret > 0){
       printMessage(receivedPacket, false, false);
       timer = chrono::system_clock::now();
       break;
       }
    }
    now = chrono::system_clock::now();
    chrono::duration<double> elapsed_seconds = now - timer; 
    
    while(elapsed_seconds.count() < 2){
        ssize_t ret = recvfrom(sockfd, buffer, PACKET_SIZE, 0 | MSG_DONTWAIT, (struct sockaddr*)&addr, &len);
        
        if(ret > 0){
	  buffer[ret] = 0;
	  receivedPacket.ConstructPacket(buffer);
	  printMessage(receivedPacket, false, false);
	  if(receivedPacket.isFIN()){
                packet ackPacket;
                //SEND ACK PACKET after receiving FIN
                SeqNum_SERVER = receivedPacket.getSeqNum();
                ackPacket.setFlag(ACK_FLAG);
                SeqNum_SERVER = (SeqNum_SERVER + 1) % 25601;
                ackPacket.setAckNum(SeqNum_SERVER + 1);
                ackPacket.setSeqNum(SeqNum_CLIENT);
                ackPacket.DeConstructPacket(send);
                printMessage(ackPacket, true, false);
                
                sendto(sockfd, send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
                
                memset((char*)&send,0,PACKET_SIZE+1);
		exit(0);
            }

            
        }
	now = chrono::system_clock::now();
	elapsed_seconds = now  - timer;

    }
}

int main(int argc, char *argv[]){
    if(argc != 4){
        cerr << "ERROR: Invalid number of arguments" << endl;
        exit(1);
    }

    
    int port = atoi(argv[2]);
    if (port < 1023 || port > 65535) {
        cerr << "ERROR: Invalid port number" << endl;
	exit(1);
    }
    
    int sockfd = socket(AF_INET, SOCK_DGRAM,0);    
    
    struct hostent *server;
    struct sockaddr_in addr;
    
    string hostname(argv[1]);
    string filename(argv[3]);
    if ((server = gethostbyname(hostname.c_str())) == NULL) {
      cerr << "ERROR: cannot get host\n";
      exit(1);
    }
    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy((char *)&addr.sin_addr.s_addr, (char *)server->h_addr,
           server->h_length);
    addr.sin_port = htons(port);
    struct stat stat_buffer;
    char* file_buffer;
    long long int size;
    if (!stat(filename.c_str(), &stat_buffer)){
      ifstream File;
      File.open(filename.c_str(), ios::in | ios::binary | ios::ate);
      if (File.is_open()){
	streampos f;
	f = File.tellg();
	size = (long long int) f;
	file_buffer = new char[size + 1];
	File.seekg(0, ios::beg);
	File.read(file_buffer, size);
	File.close();
      }
    }
	
    
    clientOpenConnection(sockfd, addr);
    sendFile(sockfd, addr, size, file_buffer);
	
    clientCloseConnection(sockfd, addr);
	
    return 0;
}
