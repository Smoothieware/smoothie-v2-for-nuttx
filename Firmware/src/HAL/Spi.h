#pragma once

#include <string>

class Spi {

public:
    Spi(int spi_channel);
    virtual ~Spi() {};
    virtual int write(int value);
    bool from_string(int spi_channel,const char *name, const char *type);
    bool lookup_pin(uint8_t port, uint8_t pin, uint8_t& func, std::string type);
private:
    bool is_spi{false}; //to read data from SPI or SSP channel
};
