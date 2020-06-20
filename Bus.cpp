#include <cstdlib>
#include "stdafx.h"
#include "Bus.h"
#include "peripheral.h"
#include "CPU_6502.h"
#include "RAM.h"

Bus::Bus()
{
    numPeripherals = 0;
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

    printf("Error: read from unmapped 0x%X\n", addr);

    return 0xFF;
}

void Bus::write(uint16_t addr, uint8_t data)
{
    for (int i = 0; i < numPeripherals; ++i)
    {
        if (addr >= peripherals[i].startAddr && addr <= peripherals[i].endAddr)
        {
            peripherals[i].pPeripheral->write(addr, data);


            if (addr >= 0x6000 && addr <= 0x6100)
            {
                //pCPU->running = false;
                printf("%s\n", &((RAM *)(peripherals[1].pPeripheral))->mem[0x6004]);
            }

            return;
        }
    }

    printf("Error: write attempt to unmapped memory 0x%X\n", addr);
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
