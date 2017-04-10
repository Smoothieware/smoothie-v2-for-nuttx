#include "IODevice.h"

class IODeviceSTDIO : public IODevice
{
public:
    IODeviceSTDIO();
    virtual ~IODeviceSTDIO();
    int write(const char *buf, size_t n);
};
