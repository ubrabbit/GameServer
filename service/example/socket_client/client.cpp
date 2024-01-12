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
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <endian.h>

#include <mutex>
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

#define MAX_TEST_PACKET_NUMBER (1 * 100)
#define PROTO_HEADER_SIZE (sizeof(int16_t) * 2)
#define PROTO_TAIL_SIZE (sizeof(size_t))

#define STATISTICS_FILENAME "statistics.txt"


void lock_file(FILE *file) {
    int fd = fileno(file);
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

void unlock_file(FILE *file) {
    int fd = fileno(file);
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

typedef struct ST_STATISTICS
{
public:
	size_t m_iTotalTTL;
	size_t m_iTotalSvrTTL;
	size_t m_iTotalSrvRecvTime;
	size_t m_iTotalClientRecvTime;
	size_t m_iTotalSrvLogicInternalCost;
	size_t m_iTotalSrvNetInternalCost;

	size_t m_iTotalPacketNum;
	size_t m_iTotalSlowNum;
	size_t m_iTotalFastNum;

	void CalcStatistics(){
		FILE *file = fopen(STATISTICS_FILENAME, "a");
		if (file == NULL) {
			perror("fopen");
			exit(EXIT_FAILURE);
		}

		lock_file(file);
		// 在这里写入数据
		fprintf(file, "总TTL<%ld>ms 服务器TTL<%ld>ms 服务器收包耗时<%ld>ms 客户端收包耗时<%ld>ms 网络线程传递到逻辑线程开销<%ld>ms 逻辑线程传递到网络线程开销<%ld>ms 总包数<%ld> 慢包数<%ld> 快包数<%ld>\n",
					m_iTotalTTL, m_iTotalSvrTTL, m_iTotalSrvRecvTime, m_iTotalClientRecvTime, m_iTotalSrvLogicInternalCost, m_iTotalSrvNetInternalCost, m_iTotalPacketNum, m_iTotalSlowNum, m_iTotalFastNum);
		unlock_file(file);
		fclose(file);
	}

}ST_STATISTICS;


typedef struct ST_PACKET
{
public:
	int32_t m_iPacketNo;
	int32_t m_iProtoNo;
	size_t m_iClientSendTime;
	size_t m_iClientRecvTime;
	bool   m_bRecvTTLSucc;

	unsigned char m_SendData[1024];
	unsigned char m_RecvData[1024];

	ST_STATISTICS m_statistics;

	void CalcTTL(ProtoHello::CSHelloRsp rstHelloRsp, size_t iServerInternalTimestamp)
	{
		if(m_iClientRecvTime <= 0)
		{
			m_bRecvTTLSucc = false;
			return;
		}
		m_bRecvTTLSucc = true;

		// 统计下收包信息
		m_statistics.m_iTotalPacketNum = 1;
		m_statistics.m_iTotalTTL = m_iClientRecvTime - m_iClientSendTime;
		m_statistics.m_iTotalSrvRecvTime = rstHelloRsp.unpack_timestamp() - m_iClientSendTime;
		m_statistics.m_iTotalClientRecvTime = m_iClientRecvTime - iServerInternalTimestamp;
		m_statistics.m_iTotalSrvLogicInternalCost = rstHelloRsp.logic_timestamp() - rstHelloRsp.unpack_timestamp();
		m_statistics.m_iTotalSrvNetInternalCost = iServerInternalTimestamp - rstHelloRsp.logic_timestamp();
		m_statistics.m_iTotalSvrTTL = iServerInternalTimestamp - rstHelloRsp.unpack_timestamp();
		if (m_statistics.m_iTotalTTL >= 100)
		{
			m_statistics.m_iTotalSlowNum += 1;
		}
		else if (m_statistics.m_iTotalTTL < 50)
		{
			m_statistics.m_iTotalFastNum += 1;
		}
	}

	void PrintTTL()
	{
		assert(m_bRecvTTLSucc);
		m_statistics.CalcStatistics();
	}

	ST_PACKET(int32_t iProtoNo, int32_t iPacketNum):
		m_iProtoNo(iProtoNo)
	{
		m_iPacketNo = iPacketNum;
	}
}ST_PACKET;

class ST_CLIENT
{
public:
	int32_t m_iSockFd;

	int32_t m_RecvedSize;
	unsigned char m_Buffer[1024];

	std::mutex m_stPacketPoolMutex; // 声明互斥锁
	map<int32_t, ST_PACKET> m_stPacketPool;

	ST_CLIENT(int32_t iSockFd):
		m_iSockFd(iSockFd)
	{
		m_RecvedSize = 0;
		memset(m_Buffer, 0, sizeof(m_Buffer));
	}

	void AddPacket(ST_PACKET& rstPacket)
	{
		std::lock_guard<std::mutex> lock(m_stPacketPoolMutex);
		m_stPacketPool.insert(pair<int32_t, ST_PACKET>(rstPacket.m_iPacketNo, rstPacket));
	}

	void Statistics()
	{
		for(auto it=m_stPacketPool.begin(); it!=m_stPacketPool.end(); it++)
		{
			ST_PACKET& rstPacket = it->second;
			rstPacket.PrintTTL();
		}
	}

	void Print()
	{
		printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>> print\n");
		for(auto it=m_stPacketPool.begin(); it!=m_stPacketPool.end(); it++)
		{
			ST_PACKET& rstPacket = it->second;
			printf("\t >>>>>>>>>>>>>>>>>>>>>>>>>>>>>> %d %s \n", rstPacket.m_iPacketNo, rstPacket.m_SendData);
		}
	}

};

static void* recv_msg(void* ptr)
{
	//printf("recv_msg start.\n");

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

			auto iPacketID = (int32_t)stHelloRsp.id();

			std::lock_guard<std::mutex> lock(rstClient.m_stPacketPoolMutex);
			auto it = rstClient.m_stPacketPool.find(iPacketID);
			if (it == rstClient.m_stPacketPool.end())
			{
				const string debug_string = stHelloRsp.content();
				printf("本地找不到ID为 <%d> 的回包数据！接收到的数据： %s", iPacketID, debug_string.c_str());
				rstClient.Print();
			}
			assert(it != rstClient.m_stPacketPool.end());

			ST_PACKET& rstPacket = it->second;
			rstPacket.m_iClientRecvTime = GetMilliSecond();

			memset(rstPacket.m_RecvData, 0, sizeof(rstPacket.m_RecvData));
			const string content_string = stHelloRsp.content();

			const char* content_str = content_string.c_str();
			memcpy(rstPacket.m_RecvData, content_str, strlen(content_str));
			//printf("1111111111111111111111: %s\n", (char*)rstPacket.m_RecvData);
			//printf("2222222222222222222222: %s\n", (char*)rstPacket.m_SendData);
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
	//printf("send_msg start.\n");

	ST_CLIENT* pstClient = (ST_CLIENT*)ptr;
	ST_CLIENT& rstClient = *pstClient;

	int32_t total = 0;
    for(int i=0; i<MAX_TEST_PACKET_NUMBER; i++)
    {
		int16_t wProtoNo = CS_PROTOCOL_MESSAGE_ID_HELLO;
		ProtoHello::CSHello stHello;
		ST_PACKET stPacket(wProtoNo, i);

		char content[512] = {'\0'};
		int idx = 0;
		// 填充250个字符
		for (int j = 0; j < 500; ++j)
		{
			content[j] = 'A';
			idx = j;
		}
		sprintf(content + idx, "%d", i);

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

		usleep(10 * 1000);
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
		//fprintf(stdout, "线程1已结束\n");
	}
	if(g_thread[1]!=0)
	{
		pthread_join(g_thread[1], NULL);
		//fprintf(stdout, "线程2已结束\n");
	}
}


int main(int argc,char *argv[]){
	const char *host = "localhost";
	int16_t port = 10002;

	int32_t sockfd = connect_remote(host, port, "tcp");

	ST_CLIENT* pstClient = new ST_CLIENT(sockfd);
	thread_create(pstClient);

	exit(0);
}
