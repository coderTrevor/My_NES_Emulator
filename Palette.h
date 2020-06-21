#pragma once
#include <stdio.h>
#include "peripheral.h"
#include "Bus.h"

typedef struct PALETTE_ENTRY
{
    uint8_t colors[4]; // Final bye can be accessed via a hack but won't be used in normal rendering
}PALETTE_ENTRY;

typedef union
{
    struct
    {
        uint8_t universalBackground;
        PALETTE_ENTRY paletteTable[8]; // 0-3 are background, 4-7 are sprites, final byte of struct is inaccessible
    };
    uint8_t allMem[0x21];   // (The last byte is inaccessible)
}PALETTE_MEM;

class Palette :
    public Peripheral
{
public:
    Palette(Bus *pPPU_Bus);
    ~Palette();

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

    PALETTE_MEM paletteMem;
};

