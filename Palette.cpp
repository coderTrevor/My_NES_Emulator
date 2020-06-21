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
    if (address < 0x3F00 || address > 0x3FFF)
    {
        printf("Bogus address 0x%X passed to palette!\n", address);
        return 0;
    }

    // Handle mirroring ("Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C")
    address &= 0x1F;
    if (address == 0x10 || address == 0x14 || address == 0x18 || address == 0x1C)
        address -= 0x10;

    return paletteMem.allMem[address];
}

void Palette::write(uint16_t address, uint8_t value)
{
    if (address < 0x3F00 || address > 0x3FFF)
    {
        printf("Bogus address 0x%X passed to palette!\n", address);
        return;
    }

    // Handle mirroring ("Addresses $3F10/$3F14/$3F18/$3F1C are mirrors of $3F00/$3F04/$3F08/$3F0C")
    address &= 0x1F;
    if (address == 0x10 || address == 0x14 || address == 0x18 || address == 0x1C)
        address -= 0x10;

    paletteMem.allMem[address] = value;
}
