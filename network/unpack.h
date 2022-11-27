#pragma once
#ifndef _GAME_NETWORK_UNPACK_H
#define _GAME_NETWORK_UNPACK_H

#include <netinet/in.h>
#include <stdint.h>
#include "defines.h"
#include "common/utils/posix.h"
#include "common/logger.h"


#pragma pack(1)

namespace network {

template<typename P, typename T>
T UnpackInt(P* buffer, T size)
{
	assert(buffer);

	T bytes= (T)0;
    size = (size > sizeof(T)) ? sizeof(T) : size;
    memcpy(&bytes, buffer, size);

	switch(size)
    {
    case 8:
        return (T)be64toh(bytes);
    case 4:
        return (T)ntohl(bytes);
    case 2:
        return (T)ntohs(bytes);
    default:
        break;
	}
	return (T)bytes;
}

template<typename P, typename T>
T PackInt(P* buffer, T value)
{
    assert(buffer);

    int32_t iSize = sizeof(T);
	switch(iSize)
    {
    case 8:
        value = htobe64((uint64_t)value);
        memcpy(buffer, &value, iSize);
        break;
    case 4:
        value = htonl(value);
        memcpy(buffer, &value, iSize);
        break;
    case 2:
        value = htons(value);
        memcpy(buffer, &value, iSize);
        break;
    default:
        memcpy(buffer, &value, iSize);
        break;
	}
	return (T)iSize;
}

template<class T, typename P>
int32_t PackStruct(int16_t wProto, T& rst, P* buffer)
{
    assert(buffer);

    wProto = htons(wProto);
    memcpy(buffer, &wProto, sizeof(int16_t));
    memcpy(buffer + sizeof(int16_t), &rst, sizeof(T));
    return (int32_t)(sizeof(int16_t) + sizeof(T));
}

template<class T, typename P>
int32_t PackProtobufStruct(int16_t wProto, T& rstProto, int32_t iBufferSize, P* buffer)
{
    assert(buffer);

    int32_t iProtoSize = (int32_t)rstProto.ByteSizeLong();
    if(iBufferSize > iProtoSize)
    {
        LOGFATAL("proto<{}> truncate by buffer_size<{}> is bigger than sizeof<{}>!", wProto, iBufferSize, iProtoSize);
        iBufferSize = iProtoSize;
    }
    rstProto.SerializeToArray(buffer, iBufferSize);
    return (int32_t)(sizeof(int16_t) + iBufferSize);
}

template<class T, typename P>
void UnpackProtobufStruct(T& rstProto, P* buffer, int32_t iBufferSize)
{
    assert(buffer);
    rstProto.ParseFromArray(buffer, iBufferSize);
    return;
}

} // namespace network

#pragma pack()

#endif
