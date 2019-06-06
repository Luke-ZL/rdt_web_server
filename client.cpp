//
//  client.cpp
//  
//
//  Created by Lihang Liu on 6/6/19.
//

#include <stdio.h>
#include <iostream>
#include <signals.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "packet.h"

using namespace std;

packet init[];
uint32_t SeqNum_SERVER
uint32_t SeqNum_CLIENT

int CWND = 512
int SSTHRESH = 5120

void printMessage(packet mPacket, bool isSend, bool isDup){
    /*
     parameters: mPacket to be sent or received
     isSent indicating if the packet is a received packet or a packet to be sent
     isDup indicating whether the packet has been sent before
     */
    
    //cwnd and ssthresh should be 0 0 since server does not implement congestion control
    
    if(isSent)
        cout << "SEND " << mPacket.getSeqNum() << " " << mPacket.getAckNum() << " " << CWND << " " << SSTHRESH;
    else
        cout << "RECV " << mPacket.getSeqNum() << " " << mPacket.getAckNum() << " " << CWND << " " << SSTHRESH;
    
    if(mPacket.isACK())
        cout << " " << "ACK";
    if(mPacket.isSYN())
        cout << " " << "SYN";
    if(mPacket.isFIN())
        cout << " " << "FIN";
    if(isSent and isDup)
        cout << " " << "DUP";
    cout << endl;
}

void clientOpenConnection(int sockfd, struct sockaddr_in &addr){
    char send[PACKET_SIZE+1];
    srand(time(NULL));

            
    packet sendingPacket;
    SeqNum_CLIENT = rand() % 25600;
    sendingPacket.setFlag(SYN_FLAG);
    sendingPacket.setAckNum(0);
    sendingPacket.setSeqNum(SeqNum_CLIENT);
    sendingPacket.DeConstructPacket(send);
    printMessage(sendingPacket, true, false);
                    
    sendto(sockfd, send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
                    
    sendingPacket.initTimer();
    sendingPacket.setSent();
}


/*

#include <iostream>
#include <vector>
#include <fstream> 
#include <sys/types.h>
#include <algorithm>
#include "packet.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
using namespace std;

int cwnd = 512;
int ssthresh = 5120;
int packet_count = 0;
vector<packet> Sender_window;
int ackNumber = 0; //sending packet's ack number, = receiving ack packet's seq number + 1
int seqNumber = 0; //sending packet's sequence number

void sendFile(int sockfd, struct sockaddr_in &addr, long long int fileSize, char* fileBuffer)
{
    socklen_t sin_size = sizeof(struct sockaddr_in);
    int lastACK = -1;
    int dupACK = -1;
    int dupACK_count = 0;
    bool FR = false;
    bool CA = false;
    char send_buf[PACKET_SIZE + 1];
    char read_buf[PACKET_SIZE + 1];
    while ((fileSize > 0) || (Sender_window.size() > 0)) {
        if (Sender_window.size() < cwnd / PAYLOAD)
        {
            packet sending_packet;
            sending_packet.setSeqNum(seqNumber);
            sending_packet.setAckNum(ackNumber);
            if (fileSize < PAYLOAD) {
                sending_packet.setPayload(fileBuffer + packet_count * PAYLOAD, (int)fileSize);
                fileSize = 0;
            }
            else {
                sending_packet.setPayload(fileBuffer + packet_count * PAYLOAD, (int)PAYLOAD);
                fileSize -= PAYLOAD;
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
                cout << "SEND " << seqNumber << ' ' << ackNumber << ' ' << cwnd << ' ' << ssthresh << endl;
                seqNumber = (seqNumber + Sender_window.back().getLength) % 25601;
                packet_count++;
                
            }
        }// end if 
        
        memset(read_buf, 0, PACKET_SIZE + 1);
        int recvlen = recvfrom(sockfd, read_buf, PACKET_SIZE, 0 | MSG_DONTWAIT,
            (struct sockaddr *)&addr, &sin_size);


        if (recvlen > 0) {
            packet receiving_packet;
            read_buf[recvlen] = 0;
            receiving_packet.ConstructPacket(read_buf);
            ackNumber = (receiving_packet.getSeqNum() + 1) % 25601;
            int Server_ack = receiving_packet.getAckNum();
            cout << "RECV" << receiving_packet.getSeqNum() << ' ' << Server_ack << ' ' << cwnd << ' ' << ssthresh;
            if (Server_ack == lastACK) {
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
                        cout << "SEND " << Sender_window[0].getSeqNum() << ' ' << receiving_packet.getSeqNum() + 1 << ' ' << cwnd << ' ' << ssthresh << ' ' << "DUP" <<endl;
                    }
                }
                else if (dupACK > 3){
                    cwnd += PAYLOAD;
                    if (cwnd > 10240) cwnd = 10240;
                }
                else break;
            }
            else {
                lastACK = Server_ack;
                dupACK_count = 0;
                if (FR) {
                    FR = false;
                    CA = true;
                    cwnd = ssthresh;
                }
                int move_count = 0;
                for (int i = 0; i < Sender_window.size(); i++) {
                    if (Sender_window[i].getSeqNum() <= Server_ack) move_count++;
                    else break;
                }
                Sender_window.erase(Sender_window.begin(), Sender_window.begin() + move_count);
                if (cwnd >= ssthresh) CA = true;
                if (CA) cwnd += PAYLOAD * PAYLOAD / cwnd;
                else cwnd += PAYLOAD;
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

    } //end while
}


*/
