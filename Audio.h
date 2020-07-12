#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <SDL.h>

#define CHANNELS                1
#define SAMPLES_PER_SECOND      44100
#define AUDIO_BUFFER_SIZE       512
#define AUDIO_SAMPLE_TYPE       int16_t

const SDL_AudioFormat AUDIO_FORMAT = AUDIO_S16;
const int BYTES_PER_SAMPLE = sizeof(AUDIO_SAMPLE_TYPE) * CHANNELS;

const double SECONDS_PER_SAMPLE = 1.0 / SAMPLES_PER_SECOND;
//const double SECONDS_PER_LINE = 0.0f;   // TODOTODOTODO
//double elapsedTime = 0.0;


// outputs at max volume
inline int16_t DoubleToSigned16(double amplitude)
{
    // Clip audio between -1 and 1
    if (amplitude <= -1.0)
        return INT16_MIN;

    if (amplitude >= 1.0)
        return INT16_MAX;

    return (int16_t)(amplitude * INT16_MAX);
}

void InitAudio();

void SendAudioData(double *pInBuffer, int sampleCount);


#ifdef __cplusplus
};
#endif /* __cplusplus */