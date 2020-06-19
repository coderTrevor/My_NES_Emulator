#pragma once
#include "peripheral.h"
#include <stdio.h>

class RAM :
    public Peripheral
{
public:
    RAM(Bus *pBus, uint16_t addressStart, uint16_t addressEnd);
    RAM(Bus *pBus, uint16_t addressStart, uint16_t addressEnd, uint16_t actualSize);
    ~RAM();

    uint8_t read(uint16_t addr) 
    {
        // TODO: This isn't working right yet
        //return mem[addr & mirrorMask];
        return mem[addr];
    }
    
    void write(uint16_t addr, uint8_t data)
    {
        //mem[addr & mirrorMask] = data;
        mem[addr] = data;
        printf("mem[0x%X] = 0x%X\n", addr, data);
    }

    void loadHexDump(char *hexDump);

    uint8_t mem[0x10000];

    // Handle mirroring smaller memory amounts to larger address spaces
    uint32_t actualSize;
    uint32_t mirrorMask;
};

