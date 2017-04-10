#include "IODeviceSTDIO.h"

#include <stdio.h>
#include <unistd.h>

IODeviceSTDIO::IODeviceSTDIO()
{}

IODeviceSTDIO::~IODeviceSTDIO()
{}

int IODeviceSTDIO::write(const char *buf, size_t n)
{
    ::write(1, buf, n);
    return n;
}
