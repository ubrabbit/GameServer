/*
	g++ client.cpp -g -o client -lpthread -std=c++11
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

#include <pthread.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#include <map>
#include<iostream>

#include "connector.h"
#include "proto/proto.h"
#include "proto/distcpp/hello.pb.h"

using namespace std;

pthread_t g_thread[2];

size_t GetMilliSecond() {
	size_t t;
#if !defined(__APPLE__) || defined(AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER)
	struct timespec ti;
	clock_gettime(CLOCK_MONOTONIC, &ti);
	t = (size_t)ti.tv_sec * 1000;
	t += ti.tv_nsec / 1000000;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	t = (size_t)tv.tv_sec * 1000;
	t += tv.tv_usec / 1000;
#endif
	return t;
}

#define MAX_TEST_PACKET_NUMBER (1 * 10000)
#define PROTO_HEADER_SIZE (sizeof(int16_t) * 2)
#define PROTO_TAIL_SIZE (sizeof(size_t))


static int32_t g_PacketNum = 0;
typedef struct ST_PACKET
{
public:
	int32_t m_iPacketNo;
	int32_t m_iProtoNo;
	size_t m_iClientSendTime;
	size_t m_iClientRecvTime;
	size_t m_iTTL;
	bool   m_bRecvTTLSucc;

	size_t m_iServerLogicInternalCost;
	size_t m_iServerNetInternalCost;
	size_t m_iServerRecvCost;
	size_t m_iServerRspCost;

	unsigned char m_SendData[256];
	unsigned char m_RecvData[256];

	void CalcTTL(ProtoHello::CSHelloRsp rstHelloRsp, size_t iServerInternalTimestamp)
	{
		if(m_iClientRecvTime <= 0)
		{
			m_bRecvTTLSucc = false;
			return;
		}
		m_bRecvTTLSucc = true;
		m_iTTL = m_iClientRecvTime - m_iClientSendTime;

		m_iServerRecvCost = rstHelloRsp.unpack_timestamp() - m_iClientSendTime;
		m_iServerRspCost = m_iClientRecvTime - rstHelloRsp.unpack_timestamp();

		m_iServerLogicInternalCost = rstHelloRsp.logic_timestamp() - rstHelloRsp.unpack_timestamp();
		m_iServerNetInternalCost = iServerInternalTimestamp - rstHelloRsp.logic_timestamp();
	}

	void PrintTTL()
	{
		assert(m_bRecvTTLSucc);
		printf("no<%d> proto<%d> send<%ld> recv<%ld> ttl<%ld>ms 服务器收包耗时<%ld>ms 服务器回包耗时<%ld>ms 网络线程传递到逻辑线程开销<%ld>ms 逻辑线程传递到网络线程开销<%ld>\n",
			 m_iPacketNo, m_iProtoNo, m_iClientSendTime, m_iClientRecvTime, m_iTTL, m_iServerRecvCost, m_iServerRspCost, m_iServerLogicInternalCost, m_iServerNetInternalCost);
	}

	ST_PACKET(int32_t iProtoNo):
		m_iProtoNo(iProtoNo)
	{
		m_iPacketNo = ++g_PacketNum;
	}
}ST_PACKET;

class ST_CLIENT
{
public:
	int32_t m_iSockFd;

	int32_t m_RecvedSize;
	unsigned char m_Buffer[1024];

	map<int32_t, ST_PACKET> m_stPacketPool;

	ST_CLIENT(int32_t iSockFd):
		m_iSockFd(iSockFd)
	{
		m_RecvedSize = 0;
		memset(m_Buffer, 0, sizeof(m_Buffer));
	}

	void AddPacket(ST_PACKET& rstPacket)
	{
		m_stPacketPool.insert(pair<int32_t, ST_PACKET>(rstPacket.m_iPacketNo, rstPacket));
	}

	void Statistics()
	{
		int32_t iTotalTTL = 0;
		int32_t iMaxPacketNo = -1;
		int32_t iMaxTTL = -1;
		for(auto it=m_stPacketPool.begin(); it!=m_stPacketPool.end(); it++)
		{
			ST_PACKET& rstPacket = it->second;
			//rstPacket.PrintTTL();

			iTotalTTL += rstPacket.m_iTTL;
			if(iMaxTTL <0 || rstPacket.m_iTTL > (size_t)iMaxTTL)
			{
				iMaxPacketNo = rstPacket.m_iPacketNo;
				iMaxTTL = rstPacket.m_iTTL;
			}
		}

		if(iMaxPacketNo > 0)
		{
			auto it = m_stPacketPool.find(iMaxPacketNo);
			if(it != m_stPacketPool.end())
			{
				printf("\n================    statistics start    ==============\n");
				(it->second).PrintTTL();
				printf("avg: %d ms\n", iTotalTTL/m_stPacketPool.size());
				printf("\n================    statistics finished ==============\n");
			}
		}
	}
};

static void* recv_msg(void* ptr)
{
	printf("recv_msg start.\n");

	ST_CLIENT* pstClient = (ST_CLIENT*)ptr;
	ST_CLIENT& rstClient = *pstClient;

	int32_t total = 0;
    while(total < MAX_TEST_PACKET_NUMBER)
    {
		int32_t iReadLen = 10240;
		if((size_t)iReadLen > (sizeof(rstClient.m_Buffer) -(size_t)rstClient.m_RecvedSize))
		{
			iReadLen = sizeof(rstClient.m_Buffer) - (size_t)rstClient.m_RecvedSize;
		}
		iReadLen = iReadLen > 0 ? iReadLen : 0;

		size_t len = read(rstClient.m_iSockFd, rstClient.m_Buffer + rstClient.m_RecvedSize, iReadLen);
		rstClient.m_RecvedSize += len;

		uint16_t wPacketSize = 0;
		// [packet_size][proto][protobuf_data][tail]
		do {
			memcpy(&wPacketSize, rstClient.m_Buffer, sizeof(uint16_t));
			wPacketSize = ntohs(wPacketSize);
			if(rstClient.m_RecvedSize < wPacketSize)
			{
				break;
			}

			uint16_t wPortoNo = 0;
			memcpy(&wPortoNo, rstClient.m_Buffer + sizeof(uint16_t), sizeof(uint16_t));
			wPortoNo = ntohs(wPortoNo);
			assert(wPortoNo == CS_PROTOCOL_MESSAGE_ID_HELLO);

			int32_t iBufferSize = wPacketSize - PROTO_HEADER_SIZE - PROTO_TAIL_SIZE;
			unsigned char* prtData =  rstClient.m_Buffer + PROTO_HEADER_SIZE;

			ProtoHello::CSHelloRsp stHelloRsp;
			stHelloRsp.ParseFromArray(prtData, iBufferSize);

			//尾部有额外的4个字节
			size_t iServerInternalTimestamp = 0;
			memcpy(&iServerInternalTimestamp, rstClient.m_Buffer + PROTO_HEADER_SIZE + iBufferSize, PROTO_TAIL_SIZE);
			iServerInternalTimestamp = be64toh(iServerInternalTimestamp);

			memmove(rstClient.m_Buffer, rstClient.m_Buffer + wPacketSize, rstClient.m_RecvedSize - wPacketSize);
			rstClient.m_RecvedSize -= wPacketSize;

			auto it = rstClient.m_stPacketPool.find((int32_t)stHelloRsp.id());
			assert(it != rstClient.m_stPacketPool.end());

			ST_PACKET& rstPacket = it->second;
			rstPacket.m_iClientRecvTime = GetMilliSecond();

			memset(rstPacket.m_RecvData, 0, sizeof(rstPacket.m_RecvData));
			const string content_string = stHelloRsp.content();

			const char* content_str = content_string.c_str();
			memcpy(rstPacket.m_RecvData, content_str, strlen(content_str));
			assert(strcmp((char*)rstPacket.m_RecvData, (char*)rstPacket.m_SendData) == 0);

			rstPacket.CalcTTL(stHelloRsp, iServerInternalTimestamp);

			total++;
		} while(rstClient.m_RecvedSize >= wPacketSize);
    }
	rstClient.Statistics();

	return NULL;
}

static void* send_msg(void* ptr)
{
	printf("send_msg start.\n");

	ST_CLIENT* pstClient = (ST_CLIENT*)ptr;
	ST_CLIENT& rstClient = *pstClient;

	int32_t total = 0;
    for(int i=0; i<MAX_TEST_PACKET_NUMBER; i++)
    {
		int16_t wProtoNo = CS_PROTOCOL_MESSAGE_ID_HELLO;
		ProtoHello::CSHello stHello;
		ST_PACKET stPacket(wProtoNo);

		char content[64] = {'\0'};
		sprintf(content, "hello_string_%d", i+1);

		stHello.set_id(stPacket.m_iPacketNo);
		stHello.set_content(content);

		int16_t wPacketSize = (int16_t)stHello.ByteSizeLong() + PROTO_HEADER_SIZE;
		char buffer[wPacketSize];
		memset(buffer, 0, sizeof(buffer));

		int16_t byte16 = htons(wPacketSize);
		memcpy(buffer, &byte16, sizeof(int16_t));

		byte16 = htons(wProtoNo);
		char* prtProto = buffer + sizeof(int16_t);
		memcpy(prtProto, &byte16, sizeof(int16_t));

		char* prtData = buffer + PROTO_HEADER_SIZE;
		stHello.SerializeToArray(prtData, (int16_t)stHello.ByteSizeLong());

		memset(stPacket.m_SendData, 0, sizeof(stPacket.m_SendData));
		memcpy(stPacket.m_SendData, content, strlen(content));
		stPacket.m_iClientSendTime = GetMilliSecond();

		rstClient.AddPacket(stPacket);
        int ret = send(rstClient.m_iSockFd, buffer, wPacketSize , 0);
        total += ret;
        //printf("ret is %d, total send is %d\n", ret, total);
    }
	return NULL;
}

void thread_create(ST_CLIENT* pstClient){
	memset(&g_thread,0,sizeof(g_thread));

    pthread_create(&g_thread[0], NULL, recv_msg, pstClient);
	pthread_create(&g_thread[1], NULL, send_msg, pstClient);

	if(g_thread[0]!=0)
	{
		pthread_join(g_thread[0], NULL);
		fprintf(stdout, "线程1已结束\n");
	}
	if(g_thread[1]!=0)
	{
		pthread_join(g_thread[1], NULL);
		fprintf(stdout, "线程2已结束\n");
	}
}

int main(int argc,char *argv[]){
	const char *host = "localhost";
	int16_t port = 10002;

	int32_t sockfd = connect_remote(host, port, "tcp");

	ST_CLIENT* pstClient = new ST_CLIENT(sockfd);
	thread_create(pstClient);

	fprintf(stdout, "connect succ!\n");

	exit(0);
}
