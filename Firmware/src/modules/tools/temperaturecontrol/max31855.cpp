/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>
#include "max31855.h"
#include "Spi.h"
#include "Pin.h"

/* define keys for the module parameters of the config file */
#define spi_channel_key "spi_channel"
#define chip_select_pin_key "chip_select_pin"
#define miso_pin_key "miso_pin"
#define sclk_pin_key "sclk_pin"

/* Initialize the module */
Max31855::Max31855():spi(nullptr){}

/* Configure the module using the parameters from the config file */
bool Max31855::configure(ConfigReader& cr, ConfigReader::section_map_t& m)
{
    /* select which SPI channel to use:
    	0: SPI
    	1: SSP0   (default)
    	2: SSP1   */
    uint8_t spi_channel = cr.get_int(m, spi_channel_key,1);
    if(spi_channel < 0 || spi_channel > 2) {
        printf("WARNING: Invalid SPI channel %d\n",spi_channel);
        return false;
    }
    delete spi;
    spi = new Spi(spi_channel);

    //Chip select can be configured to any GPIO pin
    //SPI SSEL
    this->spi_cs_pin=cr.get_string(m, chip_select_pin_key,"p1_0");
    this->spi_cs_pin.set(true);
    this->spi_cs_pin.as_output();

    //Check if the specified pins supports SPI or SSP
    //Only miso and sclk pins are needed to be initialized by the SPI mode
    //SPI MISO
    this->spi_miso_pin=cr.get_string(m, miso_pin_key,"p1_1");
    if (!(spi->from_string(spi_channel,this->spi_miso_pin,Spi::MISO))) {
        delete spi;
        return false;
    }
    //SPI SCLK
    this->spi_sclk_pin=cr.get_string(m, sclk_pin_key,"p3_0");
    if (!(spi->from_string(spi_channel,this->spi_sclk_pin,Spi::SCLK))) {
        delete spi;
        return false;
    }
    return true;
}

/* Get temperature value of the sensor */
float Max31855::get_temperature()
{
    //Initiate SPI transmission
    this->spi_cs_pin.set(false);

    //FIXME usleep() causes hardfault to the board, although data is successfully acquired without delay
    //usleep(1); // Must wait for first bit valid

    // Read 16 bits (writing something as well is required by the api)
    uint16_t data = spi->write(0);
    //  Read next 16 bits (diagnostics)
    //uint16_t data2 = spi->write(0);

    this->spi_cs_pin.set(true);
    float temperature;

    //Process temperature
    if (data & 0x0001) {
        // Error flag.
        temperature = std::numeric_limits<float>::infinity();
        // Todo: Interpret data2 for more diagnostics.
    } else {
        data = data >> 2;
        temperature = (data & 0x1FFF) / 4.f;
        if (data & 0x2000) {
            data = ~data;
            temperature = ((data & 0x1FFF) + 1) / -4.f;
        }
    }

    if (readings.size() >= readings.capacity()) {
        readings.delete_tail();
    }

    // Discard occasional errors...
    if(!isinf(temperature)) {
        readings.push_back(temperature);
    }

    // Return an average of the last readings
    if(readings.size()==0) return std::numeric_limits<float>::infinity();

    float sum = 0;
    for (int i=0; i<readings.size(); i++) {
        sum += *readings.get_ref(i);
    }

    return sum / readings.size();
}
