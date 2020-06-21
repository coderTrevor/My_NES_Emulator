#include <cstdlib>
#include "stdafx.h"
#include "Bus.h"
#include "peripheral.h"
#include "CPU_6502.h"
#include "RAM.h"
#include "System.h"

Bus::Bus()
{
    numPeripherals = 0;
    isCPU_Bus = true;
}


Bus::~Bus()
{
}

uint8_t Bus::read(uint16_t addr)
{
#ifdef SYSTEM_SIMPLE
    // 0xfe is a random byte
    if (addr == 0xfe)
        return rand() & 0xFF;
#endif

    for (int i = 0; i < numPeripherals; ++i)
    {
        if (addr >= peripherals[i].startAddr && addr <= peripherals[i].endAddr)
            return peripherals[i].pPeripheral->read(addr);
    }

    printf("Error: %s read from unmapped 0x%X\n", (isCPU_Bus ? "CPU" : "PPU"), addr);

    return 0xFF;
}

void Bus::write(uint16_t addr, uint8_t data)
{
    for (int i = 0; i < numPeripherals; ++i)
    {
        if (addr >= peripherals[i].startAddr && addr <= peripherals[i].endAddr)
        {
            peripherals[i].pPeripheral->write(addr, data);

            return;
        }
    }

    printf("Error: %s write attempt to unmapped memory 0x%X\n", (isCPU_Bus ? "CPU" : "PPU"), addr);
}

void Bus::attachPeripheral(uint16_t startAddr, uint16_t endAddr, Peripheral * pPer)
{
    if (numPeripherals == MAX_PERIPHERALS)
    {
        printf("Error: Max peripherals exceeded!\n");
        return;
    }

    peripherals[numPeripherals].startAddr = startAddr;
    peripherals[numPeripherals].endAddr = endAddr;
    peripherals[numPeripherals++].pPeripheral = pPer;
}
