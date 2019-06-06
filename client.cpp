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

