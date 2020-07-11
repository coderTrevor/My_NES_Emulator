#include <stdio.h>
#include "APU.h"
#include "Bus.h"

APU::APU(Bus *pBus) : Peripheral(pBus, 0x4000, 0x4013)
{
    // APU has two more registers to map
    pBus->attachPeripheral(0x4015, 0x4015, this);
    pBus->attachPeripheral(0x4017, 0x4017, this);

    cpuBus = pBus;

    status.entireRegister = 0;
}

APU::~APU()
{
}

uint8_t APU::read(uint16_t addr)
{
    switch (addr)
    {
        case APU_REG_STATUS:
            return status.entireRegister;
            break;

        default:
            printf("APU::read() called with unknown address, 0x%X\n", addr);
            break;
    }

    return 0;
}

void APU::write(uint16_t addr, uint8_t data)
{

    switch (addr)
    {
        case APU_REG_STATUS:
            status.entireRegister = data & APU_STATUS_WRITE_BITS;
            //printf("APU status updated: 0x%X\n", data);
            break;

        case APU_REG_PULSE1_0:
            break;
        case APU_REG_PULSE1_1:
            break;
        case APU_REG_PULSE1_2:
            break;
        case APU_REG_PULSE1_3:
            break;

        case APU_REG_PULSE2_0:
            break;
        case APU_REG_PULSE2_1:
            break;
        case APU_REG_PULSE2_2:
            break;
        case APU_REG_PULSE2_3:
            break;

        case APU_REG_TRIANGLE_0:
            break;
        case APU_REG_TRIANGLE_2_TIMER_LOW:
            break;
        case APU_REG_TRIANGLE_3:
            break;

        case APU_REG_NOISE_0:
            break;
        case APU_REG_NOISE_2_LOOP_AND_PERIOD:
            break;
        case APU_REG_NOISE_3_LENGTH_CTR_LOAD:
            break;

        case APU_REG_DMC_1:
            break;

        default:
            printf("APU::write() called with unknown address, 0x%X\n", addr);
            break;
    }
}
