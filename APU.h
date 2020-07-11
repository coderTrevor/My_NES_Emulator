#pragma once
#include <stdint.h>
#include "peripheral.h"

// PULSE 1 channel has 4 registers
#define APU_REG_PULSE1_0    0x4000
#define APU_REG_PULSE1_1    0x4001
#define APU_REG_PULSE1_2    0x4002
#define APU_REG_PULSE1_3    0x4003

// PULSE 2 channel has 4 registers
#define APU_REG_PULSE2_0    0x4004
#define APU_REG_PULSE2_1    0x4005
#define APU_REG_PULSE2_2    0x4006
#define APU_REG_PULSE2_3    0x4007

// DDlc.vvvv	Pulse 1 Duty cycle, length counter halt, constant volume/envelope flag, and volume/envelope divider period
typedef union APU_PULSE_REG_0
{
    struct
    {
        uint8_t volumeEnvelopeDividerPeriod : 4;
        bool constantVolume : 1;
        bool dontCountDown : 1; // AKA length counter halt flag 
        uint8_t dutyCycle : 2;
    };
    uint8_t entireRegister;
}APU_PULSE_REG_0;

// EPPP.NSSS	Pulse channel 1 sweep setup (write)
typedef union APU_PULSE_REG_1_SWEEP
{
    struct
    {
        uint8_t shiftAmount : 3;
        bool subtractFromPeriod : 1;
        uint8_t dividerPeriod : 3;
        bool enabled : 1;
    };
    uint8_t entireRegister;
}APU_PULSE_REG_1_SWEEP;

// 

// Triangle channel has 4 registers (but one is unused)
#define APU_REG_TRIANGLE_0              0x4008 /* CRRR RRRR  -  Length counter halt / linear counter control (C), linear counter load (R) */
#define APU_REG_TRIANGLE_1_UNUSED       0x4009
#define APU_REG_TRIANGLE_2_TIMER_LOW    0x400A
#define APU_REG_TRIANGLE_3              0x400B /* LLLL LTTT  -  Length counter load, Timer high */

// Noise channel has 4 registers (but one is unused)
#define APU_REG_NOISE_0                 0x400C /* --LC VVVV  -  Envelope loop / length counter halt (L), constant volume (C), volume/envelope (V) */
#define APU_REG_NOISE_1_UNUSED          0x400D
#define APU_REG_NOISE_2_LOOP_AND_PERIOD 0x400E /* L--- PPPP  -  Loop noise (L), noise period (P) */
#define APU_REG_NOISE_3_LENGTH_CTR_LOAD 0x400F /* LLLL L---  -  Length counter load (L) */

// DMC is mostly unimplemented
#define APU_REG_DMC_0                   0x4010
#define APU_REG_DMC_1                   0x4011
#define APU_REG_DMC_2                   0x4012
#define APU_REG_DMC_3                   0x4013

// Status and frame counter registers
#define APU_REG_STATUS          0x4015
#define APU_REG_FRAME_COUNTER   0x4017

typedef union APU_STATUS
{
    struct
    {
        bool pulse1_Enabled : 1;
        bool pulse2_Enabled : 1;
        bool triangleEnabled : 1;
        bool noiseEnabled : 1;
        bool dmcEnabled : 1;
        
        // only valid in read read
        bool unused : 1;
        bool frameInterrupt : 1;
        bool dmcInterrupt : 1;
    };

    uint8_t entireRegister;
} APU_STATUS;

#define APU_STATUS_WRITE_BITS 0x1F


class APU :
    public Peripheral
{
public:
    APU(Bus *pBus);
    ~APU();

    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);

    Bus *cpuBus;
    APU_STATUS status;
};

