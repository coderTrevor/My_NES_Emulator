#include <stdio.h>
#include "APU.h"
#include "Bus.h"
#include "Audio.h"

APU::APU(Bus *pBus) : Peripheral(pBus, 0x4000, 0x4013)
{
    // APU has two more registers to map
    pBus->attachPeripheral(0x4015, 0x4015, this);
    pBus->attachPeripheral(0x4017, 0x4017, this);

    cpuBus = pBus;

    status.entireRegister = 0;
    frameCounterStep = 0;
    apuCyclesBeforeNextFrameCounterStep = (int)APU_CYCLES_PER_QUARTER_FRAME;
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
            pulse1.reg0.entireRegister = data;
            break;
        case APU_REG_PULSE1_1:
            pulse1.reg1_Sweep.entireRegister = data;
            break;
        case APU_REG_PULSE1_2:
            pulse1.reg2_TimerLower8 = data;
            break;
        case APU_REG_PULSE1_3:
            pulse1.reg3_CounterReset_TimerHigh3.entireRegister = data;
            pulse1.pulseLengthCounter = LENGTH_LOOKUP_TABLE[pulse1.reg3_CounterReset_TimerHigh3.pulseLengthCounterLoad];
            break;

        case APU_REG_PULSE2_0:
            pulse2.reg0.entireRegister = data;
            break;
        case APU_REG_PULSE2_1:
            pulse2.reg1_Sweep.entireRegister = data;
            break;
        case APU_REG_PULSE2_2:
            pulse2.reg2_TimerLower8 = data;
            break;
        case APU_REG_PULSE2_3:
            pulse2.reg3_CounterReset_TimerHigh3.entireRegister = data;
            pulse2.pulseLengthCounter = LENGTH_LOOKUP_TABLE[pulse2.reg3_CounterReset_TimerHigh3.pulseLengthCounterLoad];
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

// Emulates the APU and sends samples to our audio subsystem in Audio.c
// timeDuration - how many seconds of audio we should generate
void APU::ProcessAudio(double timeDuration)
{
    // Process each audio channel and output the sample to the audio hardware

    // determine how many audio samples we'll be sending
    int sampleCount = (int)(timeDuration * SAMPLES_PER_SECOND) + 1;
    double *pSampleBuffer = new double[sampleCount];
    int currentSample = 0;

    // Keep generating samples until we've generated the requested duration
    // TODO: Keep track of any differences
    double apuCycles = 0.0;
    while (timeDuration >= 0.0)
    {
        while (apuCycles <= APU_CYCLES_PER_SAMPLE && timeDuration >= 0.0)
        {
            // Process pulse1 channel
            if (status.pulse1_Enabled && pulse1.pulseLengthCounter != 0)
            {
                --pulse1.timer;

                if (pulse1.timer == 0xFFFF)
                {
                    // Reset internal timer
                    pulse1.timer = pulse1.reg2_TimerLower8 | (uint16_t)pulse1.reg3_CounterReset_TimerHigh3.timerHigh3_Bits << 8;
                    pulse1.timer += 1;

                    // Advance position in duty cycle
                    pulse1.dutyCyclePosition = (pulse1.dutyCyclePosition + 1) & 7;  // 0 - 7

                    pulse1.pulseOn = DUTY_CYCLE_WAVEFORM[pulse1.reg0.dutyCycle][pulse1.dutyCyclePosition];
                }
            }
            else
                pulse1.pulseOn = false;

            // Process pulse2 channel
            if (status.pulse2_Enabled && pulse2.pulseLengthCounter != 0)
            {
                --pulse2.timer;

                if (pulse2.timer == 0xFFFF)
                {
                    // Reset internal timer
                    pulse2.timer = pulse2.reg2_TimerLower8 | (uint16_t)pulse2.reg3_CounterReset_TimerHigh3.timerHigh3_Bits << 8;
                    pulse2.timer += 1;

                    // Advance position in duty cycle
                    pulse2.dutyCyclePosition = (pulse2.dutyCyclePosition + 1) & 7;  // 0 - 7

                    pulse2.pulseOn = DUTY_CYCLE_WAVEFORM[pulse2.reg0.dutyCycle][pulse2.dutyCyclePosition];
                }
            }
            else
                pulse2.pulseOn = false;

            timeDuration -= SECONDS_PER_APU_CYCLE;
            apuCycles += 1.0;
            --apuCyclesBeforeNextFrameCounterStep;
            if (apuCyclesBeforeNextFrameCounterStep == 0)
                ClockFrameCounter();
        }
        
        if (timeDuration > 0.0)
        {
            // mix the channels and add the current sample to the output buffer
            if (pulse1.pulseOn)
                pSampleBuffer[currentSample] = 0.5;
            else
                pSampleBuffer[currentSample] = -0.5;

            if (pulse2.pulseOn)
                pSampleBuffer[currentSample] += 0.5;
            else
                pSampleBuffer[currentSample] -= 0.5;

            ++currentSample;
        }

        apuCycles -= APU_CYCLES_PER_SAMPLE;
    }

    SendAudioData(pSampleBuffer, currentSample);

    delete pSampleBuffer;
}

void APU::ClockFrameCounter()
{
    switch (frameCounterStep)
    {
        case 0:
        case 2:
            break;

        case 1:
        case 3:
            // Clock envelope and triangle counters
            // Clock lench and sweep units
            if (!pulse1.reg0.dontCountDown)
            {
                if (pulse1.pulseLengthCounter != 0)
                    --pulse1.pulseLengthCounter;
            }

            if (!pulse2.reg0.dontCountDown)
            {
                if (pulse2.pulseLengthCounter != 0)
                    --pulse2.pulseLengthCounter;
            }

            break;
    }
    
    frameCounterStep = (frameCounterStep + 1) & 3; // 0 - 3

    apuCyclesBeforeNextFrameCounterStep = (int)APU_CYCLES_PER_QUARTER_FRAME;
}
