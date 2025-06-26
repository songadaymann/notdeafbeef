#ifndef COREAUDIO_H
#define COREAUDIO_H

#include <stdint.h>

/*
 * A callback function that the user of this module provides.
 * It will be called when the audio system needs more samples.
 *
 * buffer:     An interleaved stereo float32_t buffer to be filled.
 * num_frames: The number of stereo frames requested.
 * user_data:  A pointer to whatever state the callback needs.
 */
typedef void (*audio_callback_t)(float* buffer, uint32_t num_frames, void* user_data);

/*
 * Initializes the audio playback system.
 *
 * sr:         The desired sample rate (e.g., 44100).
 * buffer_size: The number of frames per buffer chunk.
 * callback:   The function to call to generate audio.
 * user_data:  A pointer that will be passed to the callback.
 *
 * returns: 0 on success, non-zero on failure.
 */
int audio_init(uint32_t sr, uint32_t buffer_size, audio_callback_t callback, void* user_data);

/* Starts the audio playback. The program should enter a run loop after this. */
void audio_start(void);

/* Stops playback and cleans up resources. */
void audio_stop(void);

#endif /* COREAUDIO_H */ 