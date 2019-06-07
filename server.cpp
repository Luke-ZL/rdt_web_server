//
//  server.cpp
//  
//
//  Created by Lihang Liu on 6/2/19.
//


#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <vector>
#include <fstream>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "packet.h"
#include <time.h>
#include <stdlib.h>
using namespace std;

#define RECEIVER_WINDOW_SIZE 20

vector<char> TempFile; //for the temporary file
vector<packet> Receiver_window;
int FirstSeqInWindow, lastSeqinWindow;
int server_seq_count = 0; //server's ack package's seq number
int ackackNumber; //ack package's ack number
int seqNumber; //first SYN package's sequence number + 1

packet init[2];
uint32_t SeqNum_SERVER = 0;
uint32_t SeqNum_CLIENT;


void printMessage(packet mPacket, bool Sent, bool isDup){
    /*
     parameters: mPacket to be sent or received
                 isSent indicating if the packet is a received packet or a packet to be sent
                 isDup indicating whether the packet has been sent before
     */
    
    //cwnd and ssthresh should be 0 0 since server does not implement congestion control
    
    if(Sent)
        cout << "SEND " << mPacket.getSeqNum() << " " << mPacket.getAckNum() << " 0 0";
    else
        cout << "RECV " << mPacket.getSeqNum() << " " << mPacket.getAckNum() << " 0 0";
    
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

void serverOpenConnection(int sockfd, struct sockaddr_in &addr){
    packet receivedPacket;
    char buffer[PACKET_SIZE+1];
    char send[PACKET_SIZE+1];
    socklen_t len = sizeof(struct sockaddr_in);
    //srand(time(NULL));
    while(true){
        ssize_t ret = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*)&addr, &len);
        
        if(ret > 0){
            buffer[ret] = 0;
            receivedPacket.ConstructPacket(buffer);
            printMessage(receivedPacket, false, false);
            
            
            if(receivedPacket.isSYN()){
                packet sendingPacket;
                if(!init[0].isSent()){
                    SeqNum_CLIENT = receivedPacket.getSeqNum();
                    SeqNum_SERVER = rand() % 25600;
                    sendingPacket.setFlag(ACK_FLAG);
                    sendingPacket.setFlag(SYN_FLAG);
                    if(SeqNum_CLIENT + 1 > 25600)
                        SeqNum_CLIENT = 0;
                    sendingPacket.setAckNum(SeqNum_CLIENT + 1);
                    sendingPacket.setSeqNum(SeqNum_SERVER);
                    sendingPacket.DeConstructPacket(send);
                    printMessage(sendingPacket, true, false);
                    
                    sendto(sockfd, send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
                    
                    init[0] = sendingPacket;
                    sendingPacket.initTimer();
                    sendingPacket.setSent();
                }
                else{
                    packet resendPacket = init[0];
                    resendPacket.DeConstructPacket(send);
                    printMessage(resendPacket, true, true);
                    sendto(sockfd, send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
                    init[0].initTimer();
                }
            }

            memset((char*)&buffer,0,PACKET_SIZE+1);
            return;
        }
    }
}


void serverCloseConnection(int sockfd, struct sockaddr_in &addr){
    packet receivedPacket;
    char buffer[PACKET_SIZE+1];
    char send[PACKET_SIZE+1];
    socklen_t len = sizeof(struct sockaddr_in);
    //srand(time(NULL));
    while(true){
        ssize_t ret = recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*)&addr, &len);
        
        if(ret > 0){
            buffer[ret] = 0;
            receivedPacket.ConstructPacket(buffer);
            printMessage(receivedPacket, false, false);
            
            packet ackPacket;
            //SEND ACK PACKET
            SeqNum_CLIENT = receivedPacket.getSeqNum();
            ackPacket.setFlag(ACK_FLAG);
            if(SeqNum_CLIENT + 1 > 25600)
                SeqNum_CLIENT = 0;
            ackPacket.setAckNum(SeqNum_CLIENT + 1);
            ackPacket.setSeqNum(SeqNum_SERVER);
            ackPacket.DeConstructPacket(send);
            printMessage(ackPacket, true, false);
            
            sendto(sockfd, send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
            
            memset((char*)&send,0,PACKET_SIZE+1);
            
            
            //send FIN packet
            packet finPacket;
            finPacket.setFlag(FIN_FLAG);
            finPacket.setAckNum(0);
            finPacket.setSeqNum(SeqNum_SERVER);
            finPacket.DeConstructPacket(send);
            printMessage(finPacket, true, false);
            sendto(sockfd, send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
            
            
            memset((char*)&buffer,0,PACKET_SIZE+1);
        }
    }
}


/*
void sendACK(int sockfd, struct sockaddr_in addr, int ack_num, bool finpa, bool dupa) {
    packet ack_packet;
    ack_packet.setSeqNum(server_seq_count);
    ack_packet.setAckNum(ack_num);
    ack_packet.setFlag(ACK);
    char send_buf[PACKET_SIZE + 1];
    ack_packet.DeConstructPacket(send_buf);
    if (sendto(sockfd, send_buf, PACKET_SIZE, 0, (struct sockaddr *)&addr,
               sizeof(addr)) < 0) {
        cerr << "Could not send to the client" << endl;
    }
    else {
        cout << "SEND " << ack_packet.getSeqNum() << ' ' << ack_packet.getAckNum() << ' ' << 0 << ' ' << 0 << ' ' << "ACK";
        if (finpa) cout << " FIN";
        if (dupa) cout << " DUP";
        cout << endl;
    }
}
void receiveFile(int sockfd, struct sockaddr_in addr)
{
    //initialize the receiver's window
    for (int i = 0; i < RECEIVER_WINDOW_SIZE; i++)
    {
        Receiver_window.push_back(packet());
        Receiver_window[i].setSeqNum((PAYLOAD*i + seqNumber) % 25601);
    }
    FirstSeqInWindow = seqNumber % 25601;
    lastSeqinWindow = (PAYLOAD * RECEIVER_WINDOW_SIZE - PAYLOAD + seqNumber) % 25601;
    int recv_len;
    socklen_t sin_size;
    char buf[PACKET_SIZE + 1];
    while (true) {
        recv_len = recvfrom(sockfd, buf, PACKET_SIZE, 0 | MSG_DONTWAIT,
                            (struct sockaddr *)&addr, &sin_size);
        if (recv_len > 0) {
            packet received_packet;
            received_packet.ConstructPacket(buf);
            cout << "RECV " << received_packet.getSeqNum() << ' ' << received_packet.getAckNum() << ' ' << 0 << ' ' << 0;
            if (received_packet.isFIN()) {
                cout << " FIN" << endl;
                sendACK(sockfd, addr, received_packet.getSeqNum() + 1, true, false);
                server_seq_count = (server_seq_count + 1) % 25601;
                break;
            }// end if(isFIN)
            int currentSeq = received_packet.getSeqNum();
            int windowPos = (currentSeq - FirstSeqInWindow) / PAYLOAD;
            //check duplicate
            if (Receiver_window[windowPos].isAcked()) {
                server_seq_count = (server_seq_count + 1) % 25601;
                cout << " DUP" << endl;
                sendACK(sockfd, addr, ackackNumber, false, true);
                continue;
            }
            else {
                Receiver_window[windowPos] = received_packet;
                Receiver_window[windowPos].setAcked();
            }
            //move window and transfer to buffer
            int move_counter = 0;
            for (int i = 0; i < RECEIVER_WINDOW_SIZE; i++) {
                if (Receiver_window[i].isAcked()) {
                    char buffer[1024];
                    Receiver_window[i].getPayload(buffer);
                    for (int j = 0; j < Receiver_window[i].getLength(); i++) {
                        TempFile.push_back(buffer[j]);
                    }
                    move_counter++;
                } //end if
                else break;
            } //end for
            ackackNumber = (Receiver_window[move_counter - 1].getLength() + Receiver_window[move_counter - 1].getSeqNum() + 1) % 25601;
            for (int i = 0; i < move_counter; i++) {
                Receiver_window.erase(Receiver_window.begin());
                Receiver_window.push_back(packet());
                Receiver_window.end()->setSeqNum(lastSeqinWindow + (i + 1) * PAYLOAD);
            }
            FirstSeqInWindow += move_counter * PAYLOAD;
            lastSeqinWindow += move_counter * PAYLOAD;
            cout << endl;
            sendACK(sockfd, addr, ackackNumber, false, false);
            server_seq_count = (server_seq_count + 1) % 25601;
        } //end if
    } //end while
}
void assemblePackets()
{
    char* actualfile = new char[TempFile.size() + 1];
    copy(TempFile.begin(), TempFile.end(), actualfile);
    ofstream outfile("1.file", ios::binary | ios::out);
    outfile.write(actualfile, TempFile.size());
    outfile.close();
    delete actualfile;
    TempFile.clear(); //ready for next file
}

*/
int main(int argc, char *argv[]){
    if(argc != 2){
        cerr << "ERROR: Invalid arguments" << endl;
        exit(1);
    }
    
    int port = atoi(argv[1]);
    if (port < 1023 || port > 65535) {
        cerr << "ERROR: Invalid port number" << endl;
    }
    
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throwError("socket");
    
    memset((char *)&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
    
    
    serverOpenConnection(sockfd, their_addr);
    
    //receiveFile(sockfd, addr);
    
    serverCloseConnection(sockfd, their_addr);
    
    return(1);
}
