#pragma once

#include <stdio.h>

class IODevice
{
public:
    virtual ~IODevice(){};
    virtual int write(const char *buf, size_t n)= 0;
};
