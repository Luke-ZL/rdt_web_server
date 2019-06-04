//
//  server.cpp
//  
//
//  Created by Lihang Liu on 6/2/19.
//

//need a function to check if the packet sent is duplicated; what does this
//duplicated mean? sending the packet again due to timeout?

//检查一下seq和ack的关系？我感觉收到的ack应该是上一个发出去的seq+1

//第三次握手的时候如果没有payload应该怎么写？ (see line 156)

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

string requestedFile = ""

void printMessage(packet mPacket, bool isSend, bool isDup){
    /*
     parameters: mPacket to be sent or received
                 isSent indicating if the packet is a received packet or a packet to be sent
                 isDup indicating whether the packet has been sent before
     */
    
    //cwnd and ssthresh should be 0 0 since server does not implement congestion control
    
    if(isSent)
        cout << "SEND " << mPacket.getSeqNum() << " " << mPacket.getAckNum() << " 0 0";
    else
        cout << "RECV " << mPacket.getSeqNum() << " " << mPacket.getAckNum() << " 0 0";
    
    if(mPacket.isACK())
        cout << " " << "[ACK]";
    if(mPacket.isSYN())
        cout << " " << "[SYN]";
    if(mPacket.isFIN())
        cout << " " << "[FIN]";
    if(isSent and isDup)
        cout << " " << "[DUP]";
    cout << endl;
}

void openConnection(int sockfd, struct sockaddr_in &addr){
    packet receivedPacket;
    char buffer[PACKET_SIZE+1];
    char send[PACKET_SIZE+1];
    socklen_t len = sizeof(struct sockaddr_in);
    srand(time(NULL));
    while(true){
        ssize_t ret = recvfrom(sockfd, buffer, PACKET_SIZE, 0|MSG_DONTWAIT, (struct sockaddr*)&addr, &len);
        
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
                    
                    sendto(sockfd, &send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
                    
                    init[0] = sendingPacket;
                    sendingPacket.initTimer();
                    sendingPacket.setSent();
                }
                else{
                    packet resendPacket = init[0];
                    resendPacket.DeConstructPacket(send);
                    printMessage(resendPacket, true, true);
                    sendto(sockfd, &send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
                    init[0].initTimer();
                }
            }
            else if(receivedPacket.isACK() and init[0].isSent and receivedPacket.getAckNum() == init[0].getSeqNum + 1 and receivedPacket.getSeqNum == init[0].getAckNum){
                
                init[0].setAcked();
                
                int payloadLen = receivedPacket.getLength();
                if(payloadLen > 0){
                    packet third;
                    SeqNum_CLIENT = receivedPacket.getSeqNum();
                    SeqNum_SERVER = receivedPacket.getAckNum();
                    third.setFlag(ACK_FLAG);
                    if(SeqNum_CLIENT + 512 > 25600)
                        SeqNum_CLIENT = 0;
                    third.setSeqNum(SeqNum_SERVER);
                    third.setAckNum(SeqNum_CLIENT + 512);
                    for(int i = 0; i < payloadLen; i++){
                        requestedFile += (char)receivedPacket.payload[i];
                    }
                    third.DeConstructPacket(send);
                    printMessage(third, true, false);
                    sendto(sockfd, &send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
                    init[1] = third;
                    third.setSent();
                    third.initTimer();
                    return;
                }
                else{
                    //what to do to wait for 10 seconds when no payload?
                }
            }
            
            //check for timeout
            if(init[0].isSent() and !init[0].isAcked() and init[0].checkTimeout()){
                init[0].DeConstructPacket(send);
                printMessage(init[0],true,true);
                sendto(sockfd, &send, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr));
                init[0].initTimer();
            }
            memset((char*)&buf,0,PACKET_SIZE+1);
        }
    }
}


void closeConnection()
