#include "wav_writer.h"
#include <stdio.h>
#include <string.h>

static void write_le32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void write_le16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }

void write_wav(const char *path,
               const int16_t *samples,
               uint32_t frames,
               uint16_t num_channels,
               uint32_t sample_rate)
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror("write_wav: fopen");
        return;
    }

    uint16_t bits_per_sample = 16;
    uint32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
    uint16_t block_align = num_channels * bits_per_sample / 8;
    uint32_t data_chunk_size = frames * block_align;
    uint32_t riff_size = 4 + 8 + 16 + 8 + data_chunk_size; // WAVE + fmt + data

    /* RIFF header */
    fwrite("RIFF", 1, 4, f);
    write_le32(f, riff_size);
    fwrite("WAVE", 1, 4, f);

    /* fmt  sub-chunk */
    fwrite("fmt ", 1, 4, f);
    write_le32(f, 16);                 // PCM header size
    write_le16(f, 1);                  // AudioFormat = PCM
    write_le16(f, num_channels);
    write_le32(f, sample_rate);
    write_le32(f, byte_rate);
    write_le16(f, block_align);
    write_le16(f, bits_per_sample);

    /* data sub-chunk */
    fwrite("data", 1, 4, f);
    write_le32(f, data_chunk_size);
    fwrite(samples, block_align, frames, f);

    fclose(f);
} 