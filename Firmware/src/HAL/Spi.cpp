#include "Spi.h"

static const std::vector<std::tuple<uint8_t, uint8_t, uint8_t, const char*>> spi_pins {

    //SPI_MISO
    {0x03, 6,  1,  "miso0"},

    //SPI_MOSI
    {0x03, 7,  1,  "mosi0"},

    //SPI_SCLK
    {0x03, 3,  1,  "sclk0"},

    //SPI_SSEL
    {0x03, 8,  1,  "ssel0"},

    //SSP0_MISO
    {0x03, 7,  2,  "miso1"},
    {0x0F, 2,  2,  "miso1"},
    {0x01, 1,  5,  "miso1"},
    {0x03, 6,  5,  "miso1"},
    {0x09, 1,  7,  "miso1"},

    //SSP0_MOSI
    {0x03, 8,  2,  "mosi1"},
    {0x0F, 3,  2,  "mosi1"},
    {0x01, 2,  5,  "mosi1"},
    {0x03, 7,  5,  "mosi1"},
    {0x09, 2,  7,  "mosi1"},

    //SSP0_SCK
    {0x0F, 0,  0,  "sclk1"},
    {0x03, 3,  2,  "sclk1"},
    {0x03, 0,  4,  "sclk1"},

    //SSP0_SSEL
    {0x03, 6,  2,  "ssel1"},
    {0x0F, 1,  2,  "ssel1"},
    {0x01, 0,  5,  "ssel1"},
    {0x03, 8,  5,  "ssel1"},
    {0x09, 0,  7,  "ssel1"},

    //SSP1_MISO
    {0x00, 0,  1,  "miso2"},
    {0x0F, 6,  2,  "miso2"},
    {0x01, 3,  5,  "miso2"},

    //SSP1_MOSI
    {0x00, 1,  1,  "mosi2"},
    {0x0F, 7,  2,  "mosi2"},
    {0x01, 4,  5,  "mosi2"},

    //SSP1_SCK
    {0x0F, 4,  0,  "sclk2"},
    {0x01, 19, 1,  "sclk2"},

    //SSP1_SSEL
    {0x01, 20, 1,  "ssel2"},
    {0x0F, 5,  2,  "ssel2"},
    {0x01, 5,  5,  "ssel2"},
};

bool Spi::lookup_pin(uint8_t port, uint8_t pin, uint8_t& func, std::string type)
{
    for(auto &p : spi_pins) {
        if(port == std::get<0>(p) && pin == std::get<1>(p) && type == std::get<3>(p)) {
            func = std::get<2>(p);
            return true;
        }
    }
    return false;
}

bool Spi::from_string(int channel,const char *name, const char *type)
{
    // check if the pin is mappable to a SPI channel
    if(name==nullptr) //If the SPI pin is not defined
        return false;

    // pin specification
    std::string str(name);
    uint16_t port = strtol(str.substr(1).c_str(), nullptr, 16);
    size_t pos = str.find_first_of("._", 1);
    if(tolower(name[0]) != 'p' || pos == std::string::npos){
        printf("WARNING: Bad %s pin format (%s) for SPI channel %d, format is (p*_*) or (p*.*)\n",type,name,channel);
        return false;
    }
    uint16_t pin = strtol(str.substr(pos + 1).c_str(), nullptr, 10);

    // now map to a SPI output
    uint8_t func_num, modefunc;
    std::string str_type_channel = type + std::to_string(channel); //append the channel number to the type name
    if(!lookup_pin(port, pin, func_num, str_type_channel)) {
        printf("WARNING: Invalid %s pin number (%s) for SPI channel %d\n",type,name,channel);
        return false;
    }

    // TODO check if pin is already in use

    // setup pin for the SPI function
    // Set the pins for low-slew high speed output.
    if(tolower(type[0])=='m') //MISO or MOSI pins
        modefunc = SCU_MODE_INACT | SCU_MODE_INBUFF_EN | SCU_MODE_ZIF_DIS;
    if(tolower(type[0])=='s') //SCLK or SSEL pins
        modefunc = SCU_PINIO_FAST;

    Chip_SCU_PinMuxSet(port, pin, func_num|modefunc);

    return true;
}

Spi::Spi(int spi_channel)
{
    switch(spi_channel){
        case 0:
            spi = LPC_SPI;
            Chip_SPI_Init(spi);
            Chip_SPI_Int_Enable(spi);
            this->is_spi=true;
            break;
        case 1:
            ssp= LPC_SSP0;
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

int Spi::write(int value)
{
	uint16_t data;
	if(this->is_spi) //get data via SPI
	{
		while (!(spi->SR & (1 << 1)));
		spi->DR = value; //write dummy value to get data

		while (!(spi->SR & (1 << 2)));
		data= spi->DR; //read 16-bit data
	}
	else //get data via SSP
	{
		while (!(ssp->SR & (1 << 1)));
		ssp->DR = value; //write dummy value to get data

		while (!(ssp->SR & (1 << 2)));
		data = ssp->DR; //read 16-bit data
	}
    return data;
}
