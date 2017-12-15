#pragma once

#include <string>

class Spi {
    public:

        /**
         * @brief   Initialize and enable SPI or SSP peripheral
         * @param   channel     : related channel number (SPI: 0 , SSP0: 1 , SSP1: 2)
         * @return  Nothing
         */
        Spi(uint8_t spi_channel);

        /**
         * @brief   Deinitialize the SPI or SSP peripheral
         * @param   Nothing
         * @return  Nothing
         */
        virtual ~Spi() {};

        /**
         * @brief   Check if the specified pin is mappable to a SPI channel
         * @param   channel     : related channel number (SPI: 0 , SSP0: 1 , SSP1: 2)
         * @param   name        : string of the pin obtained from the config file
         * @param   type        : pin type (mosi, miso, sclk, ssel)
         * @return  true        : the pin is mappable
         * @return  false       : the pin has no SPI/SSP function
         */
        bool from_string(uint8_t channel,const char *name, uint8_t type);

        /**
         * @brief   Map the specific pin if it supports SPI or SSP peripheral
         * @param   port        : pin port number (0 to F)
         * @param   pin         : pin number (0 to 20)
         * @param   func        : function number for SPI/SSP peripheral (0 to 7)
         * @param   channel     : related channel number (SPI: 0 , SSP0: 1 , SSP1: 2)
         * @param   type        : pin type (mosi, miso, sclk, ssel)
         * @return  true        : the pin is valid
         * @return  false       : the pin has no SPI/SSP function
         * @note    Îf the specific pin is mappable, the function passes the corresponding pin function
         * value through parameter func
         */
        bool lookup_pin(uint8_t port, uint8_t pin, uint8_t& func, uint8_t channel, uint8_t type);

        /**
         * @brief   Write/read data to/from the buffer
         * @param   value       : data for writing
         * @return  received data from the buffer
         * @note    Every calling of this function returns 16-bit data only
         */
        virtual int write(int value);

        /* SPI pins */
        enum SPI {
            MISO = 0,    /**< miso pin code = 0  */
            MOSI,       /**< mosi pin code = 1 */
            SCLK,       /**< sclk pin code = 2 */
            SSEL,       /**< ssel pin code = 3 */
        };
    private:
        //to read data from either SPI or SSP peripheral
        bool is_spi{false};
};
