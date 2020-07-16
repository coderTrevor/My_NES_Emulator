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
    mutePulse1 = false;
    mutePulse2 = false;
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
            if (!status.pulse1_Enabled)
                pulse1.pulseLengthCounter = 0;
            if (!status.pulse2_Enabled)
                pulse2.pulseLengthCounter = 0;
            //printf("APU status updated: 0x%X\n", data);
            break;

        case APU_REG_PULSE1_0:
            pulse1.reg0.entireRegister = data;
            break;
        case APU_REG_PULSE1_1:
            pulse1.reg1_Sweep.entireRegister = data;
            pulse1.sweep.reloadFlag = true;
            break;
        case APU_REG_PULSE1_2:
            pulse1.reg2_TimerLower8 = data;
            pulse1.timer = pulse1.reg2_TimerLower8 | ((uint16_t)pulse1.reg3_CounterReset_TimerHigh3.timerHigh3_Bits << 8);
            break;
        case APU_REG_PULSE1_3:
            pulse1.reg3_CounterReset_TimerHigh3.entireRegister = data;

            pulse1.timer = pulse1.reg2_TimerLower8 | ((uint16_t)pulse1.reg3_CounterReset_TimerHigh3.timerHigh3_Bits << 8);
            //printf("C: 0x%X - ", pulse1.reg3_CounterReset_TimerHigh3.pulseLengthCounterLoad);
            pulse1.pulseLengthCounter = LENGTH_LOOKUP_TABLE[pulse1.reg3_CounterReset_TimerHigh3.pulseLengthCounterLoad];
            //printf("%d\n", pulse1.pulseLengthCounter);
            pulse1.dutyCyclePosition = 0;
            pulse1.envelope.startFlag = true;
            break;

        case APU_REG_PULSE2_0:
            pulse2.reg0.entireRegister = data;
            break;
        case APU_REG_PULSE2_1:
            pulse2.reg1_Sweep.entireRegister = data;
            pulse2.sweep.reloadFlag = true;
            break;
        case APU_REG_PULSE2_2:
            pulse2.reg2_TimerLower8 = data;
            pulse2.timer = pulse2.reg2_TimerLower8 | ((uint16_t)pulse2.reg3_CounterReset_TimerHigh3.timerHigh3_Bits << 8);
            break;
        case APU_REG_PULSE2_3:
            pulse2.reg3_CounterReset_TimerHigh3.entireRegister = data;

            pulse2.timer = pulse2.reg2_TimerLower8 | ((uint16_t)pulse2.reg3_CounterReset_TimerHigh3.timerHigh3_Bits << 8);
            pulse2.pulseLengthCounter = LENGTH_LOOKUP_TABLE[pulse2.reg3_CounterReset_TimerHigh3.pulseLengthCounterLoad];
            pulse2.dutyCyclePosition = 0;
            pulse2.envelope.startFlag = true;
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
            // TODO: Set envelope start flag
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

    uint16_t pulse1_TimerReset = pulse1.reg2_TimerLower8 | ((uint16_t)pulse1.reg3_CounterReset_TimerHigh3.timerHigh3_Bits << 8);
    bool pulse1_Enabled = status.pulse1_Enabled && pulse1_TimerReset >= 8;
    double pulse1_Output = 0;

    uint16_t pulse2_TimerReset = pulse2.reg2_TimerLower8 | ((uint16_t)pulse2.reg3_CounterReset_TimerHigh3.timerHigh3_Bits << 8);
    bool pulse2_Enabled = status.pulse2_Enabled && pulse2_TimerReset >= 8;
    double pulse2_Output = 0;
    
    // Keep generating samples until we've generated the requested duration
    // TODO: Keep track of any differences
    double apuCycles = 0.0;
    while (timeDuration >= 0.0)
    {
        while (apuCycles <= APU_CYCLES_PER_SAMPLE && timeDuration >= 0.0)
        {
            // Process pulse1 channel
            if (pulse1_Enabled && pulse1.pulseLengthCounter != 0 )
            {
                --pulse1.timer;

                if (pulse1.timer == 0xFFFF)
                {
                    // Reset internal timer
                    pulse1.timer = pulse1_TimerReset;
                    ++pulse1.timer;

                    // Advance position in duty cycle
                    pulse1.dutyCyclePosition = (pulse1.dutyCyclePosition + 1) & 7;  // 0 - 7

                    pulse1_Output = DUTY_CYCLE_WAVEFORM[pulse1.reg0.dutyCycle][pulse1.dutyCyclePosition];
                }
            }

            // Process pulse2 channel
            if (pulse2_Enabled && pulse2.pulseLengthCounter != 0)
            {
                --pulse2.timer;

                if (pulse2.timer == 0xFFFF)
                {
                    // Reset internal timer
                    pulse2.timer = pulse2_TimerReset;
                    ++pulse2.timer;

                    // Advance position in duty cycle
                    pulse2.dutyCyclePosition = (pulse2.dutyCyclePosition + 1) & 7;  // 0 - 7

                    pulse2_Output = DUTY_CYCLE_WAVEFORM[pulse2.reg0.dutyCycle][pulse2.dutyCyclePosition];
                }
            }

            timeDuration -= SECONDS_PER_APU_CYCLE;
            apuCycles += 1.0;
            --apuCyclesBeforeNextFrameCounterStep;
            if (apuCyclesBeforeNextFrameCounterStep == 0)
                ClockFrameCounter();
        }
        
        if (timeDuration > 0.0)
        {
            pSampleBuffer[currentSample] = 0;

            // mix the channels and add the current sample to the output buffer
            if (!mutePulse1)
            {
                if (pulse1.reg0.constantVolume)
                    pSampleBuffer[currentSample] = pulse1_Output * PULSE_VOLUME_STEP * pulse1.reg0.volumeEnvelopeDividerPeriod;
                else
                    pSampleBuffer[currentSample] = pulse1_Output * PULSE_VOLUME_STEP * pulse1.envelope.decayLevel;
            }

            if (!mutePulse2)
            {
                if (pulse2.reg0.constantVolume)
                    pSampleBuffer[currentSample] += pulse2_Output * PULSE_VOLUME_STEP * pulse2.reg0.volumeEnvelopeDividerPeriod;
                else
                    pSampleBuffer[currentSample] += pulse2_Output * PULSE_VOLUME_STEP * pulse2.envelope.decayLevel;
            }


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
            ClockPulseEnvelope(&pulse1);
            ClockPulseEnvelope(&pulse2);
            break;

        case 1:
        case 3:
            // Clock envelope and triangle counters
            ClockPulseEnvelope(&pulse1);
            ClockPulseEnvelope(&pulse2);

            // Clock length and sweep units
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

            ClockSweep(&pulse1, true);
            ClockSweep(&pulse2, false);

            break;
    }
    
    frameCounterStep = (frameCounterStep + 1) & 3; // 0 - 3

    apuCyclesBeforeNextFrameCounterStep = (int)APU_CYCLES_PER_QUARTER_FRAME;
}

void APU::ClockPulseEnvelope(APU_PULSE_CHANNEL * pChannel)
{
    // Clock envelope counters
    if (pChannel->envelope.startFlag)
    {
        pChannel->envelope.startFlag = false;
        pChannel->envelope.decayLevel = 15;
        pChannel->envelope.divider = pChannel->reg0.volumeEnvelopeDividerPeriod;
    }
    else
    {
        // envelope start flag is clear; clock the envelope divider
        if (pChannel->envelope.divider == 0)
        {
            pChannel->envelope.divider = pChannel->reg0.volumeEnvelopeDividerPeriod;

            // clock the decay level
            if (pChannel->envelope.decayLevel != 0)
            {
                --pChannel->envelope.decayLevel;
            }
            else
            {
                if (pChannel->reg0.dontCountDown)   // check envelope loop flag
                    pChannel->envelope.decayLevel = 15;
            }
        }
        else
        {
            // pChannel->envelope.divider is non-zero
            --pChannel->envelope.divider;
        }
    }
}

void APU::ClockSweep(APU_PULSE_CHANNEL * pChannel, bool channel1)
{
    /*
    When the frame counter sends a half-frame clock (at 120 or 96 Hz), two things happen.

    When the sweep unit is muting the channel, the channel's current period remains unchanged, but the divider continues to count down and reload the (unchanging) period as normal. 
    Otherwise, if the enable flag is set and the shift count is non-zero, when the divider outputs a clock, the channel's period is updated.

    If the shift count is zero, the channel's period is never updated, but muting logic still applies.
    */

    // If the divider's counter is zero, the sweep is enabled, and the sweep unit is not muting the channel: The pulse's period is adjusted.
    if(pChannel->sweep.divider == 0 && pChannel->reg1_Sweep.enabled)// && !pChannel->sweep.mutingChannel)
    {
        uint16_t pulse_TimerReset = pChannel->reg2_TimerLower8 | ((uint16_t)pChannel->reg3_CounterReset_TimerHigh3.timerHigh3_Bits << 8);

        int16_t targetTimer = pulse_TimerReset >> pChannel->reg1_Sweep.shiftAmount;
        if (pChannel->reg1_Sweep.subtractFromPeriod)
        {
            targetTimer = -targetTimer;
            if (channel1)
                targetTimer -= 1;
        }
        //printf("%d -> %d\n", pulse_TimerReset, (pulse_TimerReset + targetTimer) & APU_TIMER_VALID_BITS);
        pulse_TimerReset = (pulse_TimerReset + targetTimer) & APU_TIMER_VALID_BITS;
        pChannel->reg2_TimerLower8 = (uint8_t)(pulse_TimerReset & 0xFF);
        pChannel->reg3_CounterReset_TimerHigh3.timerHigh3_Bits = (uint8_t)((pulse_TimerReset & 0xFF00) >> 8);
    }

    // If the divider's counter is zero or the reload flag is true: The counter is set to P and the reload flag is cleared. Otherwise, the counter is decremented.
    if (pChannel->sweep.divider == 0 || pChannel->sweep.reloadFlag)
    {
        pChannel->sweep.divider = pChannel->reg1_Sweep.dividerPeriod;
        pChannel->sweep.reloadFlag = false;
    }
    else
    {
        --pChannel->sweep.divider;
    }
}
