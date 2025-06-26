#include "coreaudio.h"
#include <AudioToolbox/AudioToolbox.h>
#include <stdio.h>

#define NUM_BUFFERS 3

typedef struct {
    AudioQueueRef                queue;
    AudioQueueBufferRef          buffers[NUM_BUFFERS];
    AudioStreamBasicDescription  format;
    audio_callback_t             user_callback;
    void*                        user_data;
    uint32_t                     buffer_size_frames;
} audio_state_t;

static audio_state_t g_audio_state;

static void audio_queue_callback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer)
{
    audio_state_t* state = (audio_state_t*)inUserData;
    if (state->user_callback) {
        state->user_callback((float*)inBuffer->mAudioData, state->buffer_size_frames, state->user_data);
    }
    inBuffer->mAudioDataByteSize = state->buffer_size_frames * state->format.mBytesPerFrame;
    AudioQueueEnqueueBuffer(state->queue, inBuffer, 0, NULL);
}

int audio_init(uint32_t sr, uint32_t buffer_size, audio_callback_t callback, void* user_data)
{
    g_audio_state.user_callback = callback;
    g_audio_state.user_data = user_data;
    g_audio_state.buffer_size_frames = buffer_size;

    g_audio_state.format.mSampleRate       = sr;
    g_audio_state.format.mFormatID         = kAudioFormatLinearPCM;
    g_audio_state.format.mFormatFlags      = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    g_audio_state.format.mFramesPerPacket  = 1;
    g_audio_state.format.mChannelsPerFrame = 2;
    g_audio_state.format.mBitsPerChannel   = 32;
    g_audio_state.format.mBytesPerPacket   = sizeof(float) * 2;
    g_audio_state.format.mBytesPerFrame    = sizeof(float) * 2;

    OSStatus status = AudioQueueNewOutput(&g_audio_state.format, audio_queue_callback, &g_audio_state, NULL, NULL, 0, &g_audio_state.queue);
    if(status != noErr) { fprintf(stderr, "AudioQueueNewOutput failed\n"); return 1; }

    uint32_t buffer_size_bytes = buffer_size * g_audio_state.format.mBytesPerFrame;
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        status = AudioQueueAllocateBuffer(g_audio_state.queue, buffer_size_bytes, &g_audio_state.buffers[i]);
        if(status != noErr) { fprintf(stderr, "AudioQueueAllocateBuffer failed\n"); return 1; }
        // Prime the buffer
        audio_queue_callback(&g_audio_state, g_audio_state.queue, g_audio_state.buffers[i]);
    }
    return 0;
}

void audio_start(void)
{
    AudioQueueStart(g_audio_state.queue, NULL);
}

void audio_stop(void)
{
    AudioQueueStop(g_audio_state.queue, true);
    AudioQueueDispose(g_audio_state.queue, true);
} 