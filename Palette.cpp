#include "Palette.h"


// Attach palette data between 0x3F00 and 0x3FFF
Palette::Palette(Bus *pPPU_Bus)
    : Peripheral(pPPU_Bus, 0x3F00, 0x3FFF)
{
}


Palette::~Palette()
{
}

uint8_t Palette::read(uint16_t address)
{
    return uint8_t(0);
}
