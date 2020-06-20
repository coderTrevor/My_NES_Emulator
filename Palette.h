#pragma once
#include <stdio.h>
#include "peripheral.h"
#include "Bus.h"

class Palette :
    public Peripheral
{
public:
    Palette(Bus *pPPU_Bus);
    ~Palette();

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value)
    {
        //printf("TODO\n");
    }

    uint8_t mem[256];
};

