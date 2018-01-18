/*
      This file is part of Smoothie (http://smoothieware.org/). The motion control part is heavily based on Grbl (https://github.com/simen/grbl).
      Smoothie is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
      Smoothie is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
      You should have received a copy of the GNU General Public License along with Smoothie. If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>
#include <fcntl.h>

#include "lpc43_ssp.h"
#include "max31855.h"
#include "OutputStream.h"

#define chip_select_pin_key "chip_select_pin"
#define spi_channel_key         "spi_channel"
#define designator_key           "designator"
#define tool_id_key                 "tool_id"

//Define maximum number of max31855 sensors which may be configured
//TODO this defined number should be removed as we would like to configure any number of sensors
#define NUM_MAX31855_SENSORS 2
Max31855 *Max31855::instance[NUM_MAX31855_SENSORS];
uint8_t Max31855::instance_index = 0;
RingBuffer *Max31855::queue[NUM_MAX31855_SENSORS];

/* The following functions outside class are provided for Nuttx:
 * .select functions control GPIO Chip Select pin during data acquisition for target sensor
 * .status functions perform status operations (not currently used but need to be here)
 */

// For enabled SSP0 channel
void lpc43_ssp0select(FAR struct spi_dev_s *dev, enum spi_dev_e devid, bool selected)
{
    Max31855::instance[Max31855::instance_index]->spi_cs_pin.set(!selected);
}
uint8_t lpc43_ssp0status(FAR struct spi_dev_s *dev, enum spi_dev_e devid){return 0;}

// For enabled SSP1 channel
void lpc43_ssp1select(FAR struct spi_dev_s *dev, enum spi_dev_e devid, bool selected)
{
    Max31855::instance[Max31855::instance_index]->spi_cs_pin.set(!selected);
}
uint8_t lpc43_ssp1status(FAR struct spi_dev_s *dev, enum spi_dev_e devid){return 0;}

/* Initialize target sensor */
Max31855::Max31855(){
    this->read_flag=true;
    this->index=instance_index;
    instance[instance_index] = this;
    queue[instance_index++] = new RingBuffer(16); //buffer initialized with 16 elements
}

/* Configure the module using the parameters from the config file */
bool Max31855::configure(ConfigReader& cr, ConfigReader::section_map_t& m)
{
    /* select which SPI channel to use:
        0: SPI
        1: SSP0   (default)
        2: SSP1   */
    this->spi_channel = cr.get_int(m, spi_channel_key,1);
    if(spi_channel < 0 || spi_channel > 2) {
        printf("WARNING: Invalid SPI channel %d\n",spi_channel);
        return false;
    }

    /* open file in Nuttx corresponding to the selected SPI channel
        /dev/max31855_0: SPI
        /dev/max31855_1: SSP0   (default)
        /dev/max31855_2: SSP1   */
    std::string devpath="/dev/max31855_";
    std::string str_spi_channel=std::to_string(this->spi_channel);
    devpath+=str_spi_channel;
    this->fd = open(devpath.c_str(), O_RDONLY);
    //negative handler if can't successfully open, positive if it is open
    if (this->fd < 0) {
        printf("WARNING: Could not open file %s\n",devpath);
        return false;
    }

    //Configure GPIO chip select pin
    this->spi_cs_pin=cr.get_string(m, chip_select_pin_key,"p1_0");
    this->spi_cs_pin.set(true);
    this->spi_cs_pin.as_output();

    //Identifying tool
    this->designator=cr.get_string(m, designator_key,"");
    this->tool_id=cr.get_int(m, tool_id_key,0);

    return true;
}

/* Acquire temperature data from the sensor */
void Max31855::get_raw(OutputStream& os)
{
    //called in command thread context

    // this rate limits SPI access
    if(!this->read_flag) return;

    //Obtain temp value from SPI
    uint16_t data;
    instance_index=this->index;
    int ret=read(this->fd, &data, 2);

    //Process temp
    float temperature;
    if (ret==-1) {
        //Error
        //If enabled, error messages from Nuttx are provided here
        os.printf("On tool %s%d\n\n", this->designator.c_str(),this->tool_id);
        temperature = std::numeric_limits<float>::infinity();
        queue[instance_index]->reset();
        //TODO: Max31855 needs more diagnostics from Nuttx context, mainly SPI related
    } else {
        temperature = (data & 0x1FFF) / 4.f;
    }
    if (queue[instance_index]->full()) {
        queue[instance_index]->release_tail();
    }
    if(!isinf(temperature)) {
        queue[instance_index]->push_back(temperature);
    }

    // don't read again until get_temperature() is called
    this->read_flag=false;
}

/* Output average temperature value of the sensor */
float Max31855::get_temperature()
{
    //called in ISR context

    // allow read from hardware via SPI on next call to get_raw()
    this->read_flag=true;
    instance_index=this->index;

    // Return error
    if(queue[instance_index]->empty()) {
        return std::numeric_limits<float>::infinity();
    }

    // Return an average of the last readings
    float sum=0;
    queue[instance_index]->end_iteration();
    while(!(queue[instance_index]->is_at_head())){
        sum += *queue[instance_index]->get_ref();
        queue[instance_index]->next_iteration();
    }
    return sum / queue[instance_index]->get_size();
}
