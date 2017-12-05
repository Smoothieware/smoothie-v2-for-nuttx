/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>
#include <limits>
#include <unistd.h>
#include "Pin.h"
#include "ConfigReader.h"
#include "max31855.h"
#include "Spi.h"


#define spi_channel_key "spi_channel"
#define chip_select_pin_key "chip_select_pin"
#define mosi_pin_key "mosi_pin"
#define miso_pin_key "miso_pin"
#define sclk_pin_key "sclk_pin"

Max31855::Max31855() :
    spi(nullptr)
{
    this->read_flag=true;
}

// Get configuration from the config file
bool Max31855::configure(ConfigReader& cr, ConfigReader::section_map_t& m)
{
    /* select which SPI channel to use:
    	0: SPI
    	1: SSP0
    	2: SSP1   */
    int spi_channel = cr.get_int(m, spi_channel_key,0);
    if(spi_channel < 0 || spi_channel > 2) {
        printf("WARNING: Invalid SPI channel %d\n",spi_channel);
        return false;
    }

    spi = new Spi(spi_channel);

    //Chip select can be configured to any GPIO pin
    this->spi_cs_pin=cr.get_string(m, chip_select_pin_key,"nc"); //SPI SSEL
    this->spi_cs_pin.set(true);
    this->spi_cs_pin.as_output();

    //Check if the specified pin has SPI function
    this->spi_miso_pin=cr.get_string(m, miso_pin_key,"nc");//SPI MISO
    if (!(spi->from_string(spi_channel,this->spi_miso_pin,"miso"))) {
            delete spi;
            return false;
        }

    //Check if the specified pin has SPI function
    this->spi_sclk_pin=cr.get_string(m, sclk_pin_key,"nc"); //SPI SCLK
    if (!(spi->from_string(spi_channel,this->spi_sclk_pin,"sclk"))) {
            delete spi;
            return false;
        }
    //Only miso and sclk pins are needed to be initialized by the SPI mode

    //the module is loaded successfully
    return true;
}

// returns an average of the last few temperature values we've read
float Max31855::get_temperature()
{
	   // this rate limits SPI access
	    //if(!this->read_flag) return;

	    this->spi_cs_pin.set(false);

	    //TODO usleep() causes hardfault to the board, but data is successfully acquired without delay
	    //usleep(1); // Must wait for first bit valid

	    // Read 16 bits (writing something as well is required by the api)
	    uint16_t data = spi->write(0);
	    //  Read next 16 bits (diagnostics)
	    uint16_t data2 = spi->write(0);

	    this->spi_cs_pin.set(true);
	    printf("data=%d data2=%d data=%b data2=%b\n",data,data2,data,data2);
	    float temperature;

	    //Process temp
	    if (data & 0x0001)
	    {
	        // Error flag.
	        temperature = std::numeric_limits<float>::infinity();
	        // Todo: Interpret data2 for more diagnostics.
	    }
	    else
	    {
	        data = data >> 2;
	        temperature = (data & 0x1FFF) / 4.f;
	        if (data & 0x2000)
	        {
	            data = ~data;
	            temperature = ((data & 0x1FFF) + 1) / -4.f;
	        }
	        printf("temperature=%d\n",temperature);
	    }

	    if (readings.size() >= readings.capacity()) {
	        readings.delete_tail();
	    }

	    // Discard occasional errors...
	    if(!isinf(temperature))
	    {
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
