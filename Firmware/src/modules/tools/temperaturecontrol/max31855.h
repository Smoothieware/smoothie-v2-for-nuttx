/*
      this file is part of smoothie (http://smoothieware.org/). the motion control part is heavily based on grbl (https://github.com/simen/grbl).
      smoothie is free software: you can redistribute it and/or modify it under the terms of the gnu general public license as published by the free software foundation, either version 3 of the license, or (at your option) any later version.
      smoothie is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. see the gnu general public license for more details.
      you should have received a copy of the gnu general public license along with smoothie. if not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifndef max31855_h
#define max31855_h

#include "TempSensor.h"
#include <string>

#include "Spi.h"
#include "Pin.h"
#include "RingBuffer.h"

class Max31855 : public TempSensor
{
public:
    Max31855();
    ~Max31855() {};
    bool configure(ConfigReader& cr, ConfigReader::section_map_t& m);
    float get_temperature();
    Pin spi_cs_pin; //TODO perhaps put SPI pins inside an array or struct
    const char* spi_ssel_pin;
    const char* spi_mosi_pin;
    const char* spi_miso_pin;
    const char* spi_sclk_pin;
private:
    struct { bool read_flag:1; } ; //when true, the next call to on_idle will read a new temperature value
    Spi *spi;
    RingBuffer<float,16> readings;
};

#endif
