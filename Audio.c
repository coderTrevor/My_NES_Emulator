#include <stdio.h>
#include "Audio.h"

void InitAudio()
{
    SDL_AudioSpec AudioSettings = { 0 };

    AudioSettings.freq = SAMPLES_PER_SECOND;
    AudioSettings.format = AUDIO_FORMAT;
    AudioSettings.channels = CHANNELS;
    AudioSettings.samples = AUDIO_BUFFER_SIZE / BYTES_PER_SAMPLE;
    //AudioSettings.callback = &SDLAudioCallback;

    SDL_OpenAudio(&AudioSettings, 0);

    if (AudioSettings.format != AUDIO_FORMAT)
    {
        printf("Didn't get requested format!\n");
        SDL_Quit();
        return;
    }

    // Un-pause SDL audio (i.e. play sounds)
    SDL_PauseAudio(0);
}

float volume = 0.05;
void SendAudioData(double *pInBuffer, int sampleCount)
{
    //printf("%d %d\n", (int)pInBuffer[0], (int)pInBuffer[1]);
    uint32_t bufferSize = BYTES_PER_SAMPLE * sampleCount;
    
    AUDIO_SAMPLE_TYPE *pOutBuffer = malloc(bufferSize);

    // Convert the sample values to the proper output format
    for (int i = 0; i < sampleCount; ++i)
        pOutBuffer[i] = DoubleToSigned16(pInBuffer[i] * volume);

    // send samples to the audio hardware
    SDL_QueueAudio(1, pOutBuffer, bufferSize);

    free(pOutBuffer);
}