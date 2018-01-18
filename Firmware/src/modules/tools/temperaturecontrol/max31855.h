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

#include "Pin.h"
#include "RingBuffer.h"

class Max31855 : public TempSensor
{
    public:

        /**
         * @brief   Initialize target sensor
         * @param   Nothing
         * @return  Nothing
         * @note    The module is initialized with a null pointer
         */
        Max31855();

        /**
         * @brief   Deinitialize target sensor
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
         * @brief   Output average temperature value of the sensor
         * @param   Nothing
         * @return  Average of the last acquired temperature values
         */
        float get_temperature();

        /**
         * @brief   Acquire temperature data from the sensor
         * @param   os          : output stream
         * @return  Nothing
         * @note    Raw data is acquired and converted to temperature in Nuttx
         */
        void get_raw(OutputStream& os);

        //The following variables need to be public as Nuttx functions need access to them
        Pin spi_cs_pin;   //configure as GPIO pin
        static Max31855* instance[];
        static uint8_t instance_index;
    private:
        uint8_t spi_channel;
        uint8_t index;
        uint8_t tool_id;
        std::string designator;
        bool read_flag; //when true, the next call to get_raw will read a new temperature value
        static RingBuffer *queue[]; //buffer to store last acquired temperature values
        int fd; //handler for opening the reading file corresponding to the selected SPI channel

};

#endif
