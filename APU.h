#pragma once
#include <stdint.h>
#include "peripheral.h"
#include "Audio.h"

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

// DDlc.vvvv	Pulse Duty cycle, length counter halt, constant volume/envelope flag, and volume/envelope divider period
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

// EPPP.NSSS	Pulse channel sweep setup (write)
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

// LLLL.LLLL	Pulse timer Low 8 bits
typedef uint8_t APU_PULSE_REG_2_TIMER_LOWER_8;

// llll.lHHH	Pulse length counter load and timer High 3 bits
typedef union APU_PULSE_REG3_COUNTER_TIMER_HIGH_3
{
    struct
    {
        uint8_t timerHigh3_Bits : 3;
        uint8_t pulseLengthCounterLoad : 5;
    };
    uint8_t entireRegister;
}APU_PULSE_REG3_COUNTER_TIMER_HIGH_3;

typedef struct APU_CHANNEL_ENVELOPE
{
    uint8_t divider;
    uint8_t decayLevel;
    bool startFlag;
}APU_CHANNEL_ENVELOPE;

typedef struct APU_CHANNEL_SWEEP
{
    uint8_t divider;
    bool reloadFlag;
    bool mutingChannel;
}APU_CHANNEL_SWEEP;

typedef struct APU_PULSE_CHANNEL
{
    APU_PULSE_REG_0                     reg0;
    APU_PULSE_REG_1_SWEEP               reg1_Sweep;
    APU_PULSE_REG_2_TIMER_LOWER_8       reg2_TimerLower8;
    APU_PULSE_REG3_COUNTER_TIMER_HIGH_3 reg3_CounterReset_TimerHigh3;

    uint16_t timer;
    uint8_t pulseLengthCounter;
    uint8_t dutyCyclePosition;      // 0 - 7
    APU_CHANNEL_ENVELOPE envelope;
    APU_CHANNEL_SWEEP sweep;
    bool pulseOn;
}APU_PULSE_CHANNEL;

#define APU_TIMER_VALID_BITS                0x7FF  /* Timer is 11-bits long */

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

/*const bool DUTY_CYCLE_WAVEFORM[4][8] = { { false, true, false, false,   false, false, false, false },   // 12.5%
                                         { false, true, true, false,   false, false, false, false },    // 25%
                                         { false, true, true, true,   true, false, false, false },      // 50%
                                         { true, false, false, true,   true, true, true, true } };      // 25% negated*/

const double DUTY_CYCLE_WAVEFORM[4][8] = { { -1,  1, -1, -1, -1, -1, -1, -1 },      // 12.5%
                                           { -1,  1,  1, -1, -1, -1, -1, -1 },      // 25%
                                           { -1,  1,  1,  1,  1, -1, -1, -1 },      // 50%
                                           {  1, -1, -1,  1,  1,  1,  1,  1 } };    // 25% negated

const uint8_t LENGTH_LOOKUP_TABLE[0x20] = { 10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
                                            12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

const double APU_CYCLES_PER_SECOND = 894886.5;
const double SECONDS_PER_APU_CYCLE = 1.0 / APU_CYCLES_PER_SECOND;
const double APU_CYCLES_PER_SAMPLE = APU_CYCLES_PER_SECOND / SAMPLES_PER_SECOND;
const double SECONDS_PER_LINE = 1.0 / 60.0 / 262.0;

const double APU_CYCLES_PER_QUARTER_FRAME = APU_CYCLES_PER_SECOND / 240.0; // Approximate
const double PULSE_VOLUME_STEP = 1.0 / 30.0;

class APU :
    public Peripheral
{
public:
    APU(Bus *pBus);
    ~APU();

    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);

    void ProcessAudio(double elapsedTime);
    void ClockFrameCounter();

    void ClockPulseEnvelope(APU_PULSE_CHANNEL *pChannel);
    void ClockSweep(APU_PULSE_CHANNEL *pChannel, bool channel1);

    Bus *cpuBus;
    APU_STATUS status;
    APU_PULSE_CHANNEL pulse1;
    APU_PULSE_CHANNEL pulse2;
    int frameCounterStep; // 0 - 3
    int apuCyclesBeforeNextFrameCounterStep;
    bool mutePulse1;
    bool mutePulse2;
};

