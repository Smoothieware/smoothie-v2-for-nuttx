#pragma once

#include <string>
#include <cstring>
#include <cctype>
#include <tuple>
#include <vector>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <lpc43_spi.h>
#include <lpc43_ssp.h>
#include <lpc43_scu.h>
#include <lpc43_cgu.h>
#include "scu_18xx_43xx.h"
#include "clock_18xx_43xx.h"
#include "chip-defs.h"
#include "chip_clocks.h"
#include "spi_18xx_43xx.h"
#include "ssp_18xx_43xx.h"
#include "chip.h"
#include "Pin.h"

class Spi {

public:

    /** Create a SPI master connected to the specified pins
     *
     * Pin Options:
     *  (5, 6, 7) or (11, 12, 13)
     *
     *  mosi or miso can be specfied as NC if not used
     *
     *  @param mosi SPI Master Out, Slave In pin
     *  @param miso SPI Master In, Slave Out pin
     *  @param sclk SPI Clock pin
     */
    Spi(int spi_channel);
    virtual ~Spi() {};
    /** Configure the data transmission format
     *
     *  @param bits Number of bits per SPI frame (4 - 16)
     *  @param mode Clock polarity and phase mode (0 - 3)
     *
     * @code
     * mode | POL PHA
     * -----+--------
     *   0  |  0   0
     *   1  |  0   1
     *   2  |  1   0
     *   3  |  1   1
     * @endcode
     */
   // void format(int bits, int mode = 0);

    /** Set the spi bus clock frequency
     *
     *  @param hz SCLK frequency in hz (default = 1MHz)
     */
    //void frequency(int hz = 1000000);

    /** Write to the SPI Slave and return the response
     *
     *  @param value Data to be sent to the SPI slave
     *
     *  @returns
     *    Response from the SPI slave
    */
    virtual int write(int value);
    bool from_string(int spi_channel,const char *name, const char *type);
    bool lookup_pin(uint8_t port, uint8_t pin, uint8_t& func, std::string type);
protected:
    struct { bool is_spi:1; } ; //to read data from SPI or SSP channel
    LPC_SPI_T* spi;
    LPC_SSP_T* ssp;
};
