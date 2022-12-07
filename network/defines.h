#pragma once

#ifndef _GAME_NETWORK_DEFINES_H
#define _GAME_NETWORK_DEFINES_H

#include <stdlib.h>
#include "common/logger.h"
#include "common/defines.h"

namespace network {

typedef unsigned char BYTE;

#define NETWORK_SERVER_IP_ADDRESS_LEN      (64)        // IP地址最大长度
#define NETWORK_SERVER_LISTEN_QUEUE_LEN    (512)       // accept queue最大长度

#define NETWORK_PACKET_BUFFER_DEFAULT_SIZE (128)       // 单包默认长度
#define NETWORK_PACKET_BUFFER_READ_SIZE    (10*1024)   // 单次收包最大长度1KB

// 网络收包缓冲区总大小
#define NETWORK_PACKET_BUFFER_POOL_SIZE (1 * 512)

#define NETWORK_PACKET_HEADER_SIZE         ((int32_t)sizeof(int16_t) * 2)         // 包头长度
#define NETWORK_PACKET_TAIL_SIZE           ((int32_t)sizeof(size_t))              // 包尾长度

enum E_NETWORK_NET_MODE_TYPE
{
    E_NETWORK_NET_MODE_LIBEVENT = 1,
    E_NETWORK_NET_MODE_SOCKET   = 2,
};
#define NETWORK_NET_MODE_SELECTED (1)

enum E_NETWORK_SERVER_IP_PORT_RANGE
{
    E_NETWORK_SERVER_IP_PORT_RANGE_INVALID = 1000,
    E_NETWORK_SERVER_IP_PORT_RANGE_MAX = 65536
};

struct ST_ServerIPInfo
{
    short m_wServerPort;
    char m_achServerIP[NETWORK_SERVER_IP_ADDRESS_LEN];

    ST_ServerIPInfo()
    {
        memset(this, 0, sizeof(*this));
    }

    bool Init(short m_wPort, const char* pchServerIP)
    {
        if(NULL == pchServerIP)
        {
            LOGCRITICAL("ServerIP is NULL, invalid!");
            return false;
        }

        if(m_wPort <= E_NETWORK_SERVER_IP_PORT_RANGE_INVALID || m_wPort >= E_NETWORK_SERVER_IP_PORT_RANGE_MAX)
        {
            LOGCRITICAL("ServerIPPort {} invalid!", m_wPort);
            return false;
        }

        m_wServerPort = m_wPort;
        strncpy(m_achServerIP, pchServerIP, sizeof(m_achServerIP));
        return true;
    }

};

enum E_NETWORK_CMD_REQUEST_LIST
{
    NETWORK_CMD_REQUEST_SEND_ALL_PACKET = 'S',
    NETWORK_CMD_REQUEST_CLOSE_SERVER    = 'C'
};

#define ST_NETWORK_CMD_REQUEST_MAX_SIZE (32)
struct ST_NETWORK_CMD_REQUEST
{
    char m_Header[2];
    union
    {
        char buffer[ST_NETWORK_CMD_REQUEST_MAX_SIZE-sizeof(m_Header)];
    } m_Body;

    void construct()
    {
        ASSERT_STRUCT_SIZE_EQUAL_MACRO(*this, ST_NETWORK_CMD_REQUEST_MAX_SIZE);
        memset(this, 0, sizeof(*this));
    }

    ST_NETWORK_CMD_REQUEST()
    {
        construct();
    }

    ST_NETWORK_CMD_REQUEST(const char chCmd)
    {
        construct();
        m_Header[0] = chCmd;
    }

    BYTE GetCmd()
    {
        return (BYTE)m_Header[0];
    }

};

} // namespace network

#endif
