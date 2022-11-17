#pragma once

#include "noncopyable.h"

template<class T>
class Singleton: public noncopyable
{
public:
    static T& Instance()
    {
        static T m_sInstance;
        return m_sInstance;
    }
};
