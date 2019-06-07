#include "macros.h"
#include <stdint.h>
#include <string.h>
#include <chrono> 

#define ACK_FLAG 0
#define SYN_FLAG 1
#define FIN_FLAG 2


class packet {
private:
	struct header {
		uint32_t SeqNum;
		uint32_t AckNum;
	    uint16_t len;
		uint8_t flags;
		uint8_t padding = 0;
	}; //12 bytes in total
	header hd;
	char payload[PAYLOAD];
	bool sent;
	bool acked;
	int payload_length;
	std::chrono::time_point<std::chrono::system_clock> start;

public:
	//default constructor
	packet() {
		memset(payload, 0, PAYLOAD);
		memset(&hd, 0, sizeof(hd));
		sent = acked = false;
		payload_length = 0;
	}

	//constructor with size 
	packet(int size) {
		memset(payload, 0, PAYLOAD);
		memset(&hd, 0, sizeof(hd));
		sent = acked = false;
		payload_length = size;
	}



	//wrapper for getting and settingprivate variables
	bool isSent() {
		return sent;
	}

	bool isAcked() {
		return acked;
	}

	int getFlag() {
		return hd.flags;
	}

	int getLength() {
		return payload_length;
	}

	uint32_t getSeqNum() {
		return hd.SeqNum;
	}

	uint32_t getAckNum() {
		return hd.AckNum;
	}

	void getPayload(char* buf){
		memset(buf, 0, sizeof(buf));
		for (int i = 0; i < payload_length; i++) {
			buf[i] = payload[i];
		}
	}

	bool isACK() {
        return (hd.flags & ACK);
	}

	bool isSYN() {
        return (hd.flags & SYN);
	}

	bool isFIN() {
        return (hd.flags & FIN);
	}

	void setFlag(int f) {
		switch (f) {
		case ACK_FLAG:
			hd.flags = hd.flags | ACK;
			break;
        case SYN_FLAG:
			hd.flags = hd.flags | SYN;
			break;
		case FIN_FLAG:
			hd.flags = hd.flags | FIN;
			break;
		default:
			break;
		}
	}

	void setSeqNum(uint32_t n) {
		hd.SeqNum = n;
	}

	void setAckNum(uint32_t a) {
		hd.AckNum = a;
	}

	void setSent() {
		sent = true;
	}

	void setAcked() {
		acked = true;
	}

	void setPayload(char* buf, int len) {
		hd.len = payload_length = len;
		for (int i = 0; i < len; i++) {
			payload[i] = buf[i];
		}
	}
	//resetData

	//resetAcked

	void ConstructPacket(char* buf) {
	        hd.SeqNum = (uint32_t)((((uint8_t)buf[0] << 24) | ((uint8_t)buf[1] << 16)) | ((uint8_t)buf[2] << 8)) | (uint8_t)buf[3];
		hd.AckNum = (uint32_t)((((uint8_t)buf[4] << 24) | ((uint8_t)buf[5] << 16)) | ((uint8_t)buf[6] << 8)) | (uint8_t)buf[7];
		hd.flags = (uint8_t)buf[10];
		hd.len = payload_length = ((uint8_t)buf[8] << 8) | (uint8_t)buf[9];
		for (int i = 0; i < payload_length; i++) {
			payload[i] = buf[12 + i];
		}
	}

	void DeConstructPacket(char* buf) {
		memset(buf, 0, sizeof(buf));

		buf[0] = (hd.SeqNum >> 24) & 0xFF;
		buf[1] = (hd.SeqNum >> 16) & 0xFF;
		buf[2] = (hd.SeqNum >> 8) & 0xFF;
		buf[3] = hd.SeqNum & 0xFF;
		buf[4] = (hd.AckNum >> 24) & 0xFF;
		buf[5] = (hd.AckNum >> 16) & 0xFF;
		buf[6] = (hd.AckNum >> 8) & 0xFF;
		buf[7] = hd.AckNum & 0xFF;
		buf[8]= (hd.len >> 8) & 0xFF;
		buf[9]= hd.len & 0xFF;
		buf[10] = hd.flags & 0xFF;
		memcpy(buf + 12, payload, hd.len);
		payload_length = hd.len;
	}

	//copy operator overload
	packet &operator=(const packet &other) {
		if (this != &other) {
			hd = other.hd;
			sent = other.sent;
			acked = other.acked;
			payload_length = other.payload_length;
			start = other.start;
			for (int i = 0; i < payload_length; i++) {
				payload[i] = other.payload[i];
			}
		}
		return *this;
	}

	void initTimer() {
		start = std::chrono::system_clock::now();
	}

	//idea taken from https://www.geeksforgeeks.org/chrono-in-c/
	bool checkTimeout() {
		std::chrono::time_point<std::chrono::system_clock> end;
		end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		return (elapsed_seconds.count() > RTO);
	}
};

