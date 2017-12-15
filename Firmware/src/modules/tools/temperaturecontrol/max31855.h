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

#include "Pin.h"
#include "RingBuffer.h"

class Spi;

class Max31855 : public TempSensor
{
    public:

        /**
         * @brief   Initialize the module
         * @param   Nothing
         * @return  Nothing
         * @note    The module is initialized with a null pointer
         */
        Max31855();

        /**
         * @brief   Deinitialize the module
         * @param   Nothing
         * @return  Nothing
         */
        ~Max31855() {};

        /**
         * @brief   Configure the module using the parameters from the config file
         * @param   cr          : target config file object
         * @param   m           : target section map of the config file
         * @return  false       : the module failed to be configured
         * @return  true        : the module is configured and loaded successfully
         */
        bool configure(ConfigReader& cr, ConfigReader::section_map_t& m);

        /**
         * @brief   Acquire temperature data of the sensor
         * @param   Nothing
         * @return  Average of the last acquired temperature values
         * @note    Data is acquired and converted to temperature value
         * and also depends on the readings per second parameter
         */
        float get_temperature();
    private:
        Spi *spi;                      //pointer to SPI
        Pin spi_cs_pin;                //configure as GPIO pin
        const char* spi_miso_pin;      //configure as SPI pin
        const char* spi_sclk_pin;      //configure as SPI pin
        RingBuffer<float,16> readings; //number of readings per second
};

#endif
