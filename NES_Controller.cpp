#include <stdio.h>
#include "NES_Controller.h"


NES_Controller::NES_Controller(Bus *pBus) : Peripheral(pBus, 0x4016, 0x4017)
{
    buttons.allBits = 0;
}


NES_Controller::~NES_Controller()
{
}

uint8_t NES_Controller::read(uint16_t address)
{
    if (address != 0x4016 && address != 0x4017)
    {
        printf("Invalid address 0x%X for controller\n", address);
        return 0;
    }

    //printf("Controller read\n");

    uint8_t value = 0;

    // return lowest bit of latch
    if (latch & 1)
        value = 1;

    // shift the next bit of input into latch value
    latch >>= 1;

    //printf("value: 0x%X\n\n", value);

    return value;
}

void NES_Controller::write(uint16_t address, uint8_t value)
{
    //printf("Controller write : %d\n", value);
    if (address != 0x4016 && address != 0x4017)
    {
        printf("Invalid address 0x%X for controller\n", address);
        return;
    }

    // latch in button valuesx
    latch = buttons.allBits;

    //if((value & 1) == 0)
    //    buttons.allBits = 0;

    //printf("latch value: 0x%X\n", latch);
}
