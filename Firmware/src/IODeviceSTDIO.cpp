#include "IODeviceSTDIO.h"

#include <stdio.h>

IODeviceSTDIO::IODeviceSTDIO()
{}

IODeviceSTDIO::~IODeviceSTDIO()
{}

int IODeviceSTDIO::write(const char *buf, size_t n)
{
    puts(buf);
    return n;
}
