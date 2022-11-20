#pragma once

#include <string>
#include <endian.h>

inline int is_big_endian(void)
{
    union {
        uint32_t i;
        char c[4];
    } e = { 0x01000000 };

    return e.c[0];
}

struct in_addr GetHostByName(const std::string &host);
uint64_t GetThreadId();
bool CreatePipe(int32_t& iFdRead, int32_t& iFdWrite);
