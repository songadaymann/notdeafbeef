#ifndef WAV_WRITER_H
#define WAV_WRITER_H

#include <stdint.h>

/*
 * Write a little-endian 16-bit PCM WAV file.
 * path         — output filename
 * samples      — interleaved PCM samples (len = frames * channels)
 * frames       — number of audio frames
 * num_channels — 1 = mono, 2 = stereo
 * sample_rate  — e.g., 44100
 */
void write_wav(const char *path,
               const int16_t *samples,
               uint32_t frames,
               uint16_t num_channels,
               uint32_t sample_rate);

#endif /* WAV_WRITER_H */ 