#include "Spi.h"

#include <string>
#include <cctype>
#include <tuple>
#include <vector>

#include "lpc_types.h"
#include "chip-defs.h"
#include "spi_18xx_43xx.h"
#include "ssp_18xx_43xx.h"
#include "scu_18xx_43xx.h"

/* Pointers to SPI/SSP register block structure */
LPC_SPI_T* spi;
LPC_SSP_T* ssp;

/* Pin map for SPI/SSP0/SSP1 {port number, pin number, function number, channel number, pin type} */
static const std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t,uint8_t>> spi_pins {
    //SPI_MISO
    {0x03, 6,  1,  0,  Spi::MISO},
    //SSP0_MISO
    {0x03, 7,  2,  1,  Spi::MISO},
    {0x0F, 2,  2,  1,  Spi::MISO},
    {0x01, 1,  5,  1,  Spi::MISO},
    {0x03, 6,  5,  1,  Spi::MISO},
    {0x09, 1,  7,  1,  Spi::MISO},
    //SSP1_MISO
    {0x00, 0,  1,  2,  Spi::MISO},
    {0x0F, 6,  2,  2,  Spi::MISO},
    {0x01, 3,  5,  2,  Spi::MISO},
    //SPI_MOSI
    {0x03, 7,  1,  0,  Spi::MOSI},
    //SSP0_MOSI
    {0x03, 8,  2,  1,  Spi::MOSI},
    {0x0F, 3,  2,  1,  Spi::MOSI},
    {0x01, 2,  5,  1,  Spi::MOSI},
    {0x03, 7,  5,  1,  Spi::MOSI},
    {0x09, 2,  7,  1,  Spi::MOSI},
    //SSP1_MOSI
    {0x00, 1,  1,  2,  Spi::MOSI},
    {0x0F, 7,  2,  2,  Spi::MOSI},
    {0x01, 4,  5,  2,  Spi::MOSI},
    //SPI_SCLK
    {0x03, 3,  1,  0,  Spi::SCLK},
    //SSP0_SCLK
    {0x0F, 0,  0,  1,  Spi::SCLK},
    {0x03, 3,  2,  1,  Spi::SCLK},
    {0x03, 0,  4,  1,  Spi::SCLK},
    //SSP1_SCLK
    {0x0F, 4,  0,  2,  Spi::SCLK},
    {0x01, 19, 1,  2,  Spi::SCLK},
    //SPI_SSEL
    {0x03, 8,  1,  0,  Spi::SSEL},
    //SSP0_SSEL
    {0x03, 6,  2,  1,  Spi::SSEL},
    {0x0F, 1,  2,  1,  Spi::SSEL},
    {0x01, 0,  5,  1,  Spi::SSEL},
    {0x03, 8,  5,  1,  Spi::SSEL},
    {0x09, 0,  7,  1,  Spi::SSEL},
    //SSP1_SSEL
    {0x01, 20, 1,  2,  Spi::SSEL},
    {0x0F, 5,  2,  2,  Spi::SSEL},
    {0x01, 5,  5,  2,  Spi::SSEL},
};

/* Initialize and enable SPI or SSP peripheral*/
Spi::Spi(uint8_t spi_channel)
{
    switch(spi_channel) {
        case 0:
            spi = LPC_SPI;
            Chip_SPI_Init(spi);
            Chip_SPI_Int_Enable(spi);
            this->is_spi=true;
            break;
        case 1:
            ssp = LPC_SSP0;
            Chip_SSP_Init(ssp);
            Chip_SSP_Enable(ssp);
            break;
        case 2:
            ssp= LPC_SSP1;
            Chip_SSP_Init(ssp);
            Chip_SSP_Enable(ssp);
            break;
    }
}

/* Check if the specific pin is mappable to a SPI channel */
bool Spi::from_string(uint8_t channel,const char *name, uint8_t type)
{
    //If the SPI pin is not defined
    if(name==nullptr) {
        return false;
    }
    // Check the pin specification obtained from the config file
    std::string str(name);
    uint16_t port = strtol(str.substr(1).c_str(), nullptr, 16);
    size_t pos = str.find_first_of("._", 1);
    if(tolower(name[0]) != 'p' || pos == std::string::npos) {
        printf("WARNING: Bad pin format (%s) for SPI channel %d, format is (p*_*) or (p*.*)\n",name,channel);
        return false;
    }
    uint16_t pin = strtol(str.substr(pos + 1).c_str(), nullptr, 10);

    // now map to a SPI output
    uint8_t func, mode;
    if(!lookup_pin(port, pin, func, channel, type)) {
        printf("WARNING: Invalid pin number (%s) for SPI channel %d\n",name,channel);
        return false;
    }

    // TODO check if pin is already in use

    // Set the pins for low-slew high speed output mode
    if(type == MISO || type == MOSI) {
        mode = SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS;
    }
    if(type == SCLK || type == SSEL ) {
        mode = SCU_PINIO_FAST;
    }
    // setup pin for the SPI function and mode
    Chip_SCU_PinMuxSet(port, pin, func|mode);

    return true;
}

/* Map the specific pin if it supports SPI or SSP peripheral */
bool Spi::lookup_pin(uint8_t port, uint8_t pin, uint8_t& func, uint8_t channel, uint8_t type)
{
    for(auto &p : spi_pins) {
        if(port == std::get<0>(p) && pin == std::get<1>(p) && channel == std::get<3>(p) && type == std::get<4>(p)) {
            func = std::get<2>(p);
            return true;
        }
    }
    return false;
}

/* Write/read data to/from the buffer */
int Spi::write(int value)
{
    uint16_t data; //16-bit variable for receiving data
    if(this->is_spi) { //via SPI peripheral
        while (!(spi->SR & (1 << 1)));
        spi->DR = value; //write
        while (!(spi->SR & (1 << 2)));
        data= spi->DR; //read
    } else { //via SSP peripheral
        while (!(ssp->SR & (1 << 1)));
        ssp->DR = value; //write
        while (!(ssp->SR & (1 << 2)));
        data = ssp->DR; //read
    }
    return data;
}
