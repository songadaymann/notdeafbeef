#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>
#include "../include/visual_types.h"

// WAV file header structure
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t chunk_size;    // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // Format chunk size
    uint16_t audio_format;  // Audio format (1 = PCM)
    uint16_t num_channels;  // Number of channels
    uint32_t sample_rate;   // Sample rate
    uint32_t byte_rate;     // Byte rate
    uint16_t block_align;   // Block align
    uint16_t bits_per_sample; // Bits per sample
    char data[4];           // "data"
    uint32_t data_size;     // Data size
} wav_header_t;

// Audio analysis data
typedef struct {
    int16_t *samples;       // Raw audio samples (stereo interleaved)
    uint32_t sample_count;  // Total samples (left + right)
    uint32_t sample_rate;   // Sample rate (44100)
    uint16_t channels;      // Number of channels (2 for stereo)
    uint16_t bits_per_sample; // Bits per sample (16)
    
    // Analysis data
    float *rms_levels;      // RMS levels per video frame
    uint32_t num_frames;    // Number of video frames
    float duration_sec;     // Total duration in seconds
    float bpm;              // Detected BPM (if available)
} audio_data_t;

static audio_data_t audio_data = {0};
static bool audio_loaded = false;
static SDL_AudioDeviceID audio_device = 0;
static uint32_t audio_position = 0; // Current playback position in samples
static uint32_t loop_point_samples = 0; // Where to loop back to (before delay tail)
static uint32_t musical_content_samples = 0; // Length of actual musical content

// Audio callback function for SDL2 with seamless looping
static void audio_callback(void *userdata, Uint8 *stream, int len) {
    (void)userdata; // Unused
    
    if (!audio_loaded) {
        memset(stream, 0, len);
        return;
    }
    
    int samples_needed = len / sizeof(int16_t);
    int16_t *output = (int16_t *)stream;
    
    for (int i = 0; i < samples_needed; i++) {
        if (audio_position < musical_content_samples) {
            // Normal playback of musical content
            output[i] = audio_data.samples[audio_position];
            audio_position++;
        } else {
            // Loop back to the beginning when musical content ends
            audio_position = loop_point_samples;
            output[i] = audio_data.samples[audio_position];
            audio_position++;
        }
    }
}

// Load WAV file and extract audio data
bool load_wav_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Error: Could not open WAV file: %s\n", filename);
        return false;
    }
    
    wav_header_t header;
    size_t read_bytes = fread(&header, sizeof(header), 1, file);
    if (read_bytes != 1) {
        printf("Error: Could not read WAV header\n");
        fclose(file);
        return false;
    }
    
    // Validate WAV header
    if (strncmp(header.riff, "RIFF", 4) != 0 || 
        strncmp(header.wave, "WAVE", 4) != 0 ||
        strncmp(header.fmt, "fmt ", 4) != 0 ||
        strncmp(header.data, "data", 4) != 0) {
        printf("Error: Invalid WAV file format\n");
        fclose(file);
        return false;
    }
    
    // Store audio parameters
    audio_data.sample_rate = header.sample_rate;
    audio_data.channels = header.num_channels;
    audio_data.bits_per_sample = header.bits_per_sample;
    audio_data.sample_count = header.data_size / (header.bits_per_sample / 8);
    audio_data.duration_sec = (float)audio_data.sample_count / header.num_channels / header.sample_rate;
    
    printf("WAV Info: %d Hz, %d channels, %d bits, %.2f seconds\n", 
           audio_data.sample_rate, audio_data.channels, 
           audio_data.bits_per_sample, audio_data.duration_sec);
    
    // Only support 16-bit stereo for now
    if (header.bits_per_sample != 16 || header.num_channels != 2) {
        printf("Error: Only 16-bit stereo WAV files supported\n");
        fclose(file);
        return false;
    }
    
    // Allocate memory for samples
    audio_data.samples = malloc(header.data_size);
    if (!audio_data.samples) {
        printf("Error: Could not allocate memory for audio samples\n");
        fclose(file);
        return false;
    }
    
    // Read audio data
    read_bytes = fread(audio_data.samples, header.data_size, 1, file);
    if (read_bytes != 1) {
        printf("Error: Could not read audio data\n");
        free(audio_data.samples);
        fclose(file);
        return false;
    }
    
    fclose(file);
    
    // Calculate RMS levels per video frame
    audio_data.num_frames = (uint32_t)(audio_data.duration_sec * VIS_FPS);
    audio_data.rms_levels = malloc(audio_data.num_frames * sizeof(float));
    if (!audio_data.rms_levels) {
        printf("Error: Could not allocate memory for RMS levels\n");
        free(audio_data.samples);
        return false;
    }
    
    // Calculate RMS for each video frame
    uint32_t samples_per_frame = audio_data.sample_rate / VIS_FPS;
    for (uint32_t frame = 0; frame < audio_data.num_frames; frame++) {
        uint32_t start_sample = frame * samples_per_frame * audio_data.channels;
        uint32_t end_sample = start_sample + samples_per_frame * audio_data.channels;
        
        if (end_sample > audio_data.sample_count) {
            end_sample = audio_data.sample_count;
        }
        
        // Calculate RMS for this frame
        float sum_squares = 0.0f;
        uint32_t sample_count = 0;
        
        for (uint32_t i = start_sample; i < end_sample; i++) {
            float sample = audio_data.samples[i] / 32768.0f; // Normalize to -1.0 to 1.0
            sum_squares += sample * sample;
            sample_count++;
        }
        
        if (sample_count > 0) {
            audio_data.rms_levels[frame] = sqrtf(sum_squares / sample_count);
        } else {
            audio_data.rms_levels[frame] = 0.0f;
        }
    }
    
    // Try to extract BPM from filename (our audio system outputs BPM info)
    audio_data.bpm = 120.0f; // Default fallback
    
    printf("Audio analysis complete: %d frames, %.3f avg RMS\n", 
           audio_data.num_frames, audio_data.rms_levels[0]);
    
    // Initialize SDL audio for playback
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            printf("Warning: Could not initialize SDL audio: %s\n", SDL_GetError());
            audio_loaded = true;
            return true; // Continue without audio playback
        }
    }
    
    SDL_AudioSpec want, have;
    want.freq = audio_data.sample_rate;
    want.format = AUDIO_S16SYS;
    want.channels = audio_data.channels;
    want.samples = 4096;
    want.callback = audio_callback;
    want.userdata = NULL;
    
    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device == 0) {
        printf("Warning: Could not open audio device: %s\n", SDL_GetError());
        audio_loaded = true;
        return true; // Continue without audio playback
    }
    
    printf("Audio playback initialized: %d Hz, %d channels\n", have.freq, have.channels);
    
    // Calculate musical content length based on BPM and structure  
    // Our audio system generates 8 bars of music, but let's use a more conservative estimate
    // The delay tail is probably the last 1.5-2 seconds, so loop the first ~7.5 seconds
    float musical_duration = audio_data.duration_sec * 0.8f; // Use 80% of total duration
    
    musical_content_samples = (uint32_t)(musical_duration * audio_data.sample_rate * audio_data.channels);
    
    // Make sure we don't exceed the actual audio length
    if (musical_content_samples > audio_data.sample_count) {
        musical_content_samples = audio_data.sample_count;
    }
    
    loop_point_samples = 0; // Loop back to the very beginning
    
    printf("Musical content: %.2f seconds (%d samples), Full audio: %.2f seconds\n", 
           musical_duration, musical_content_samples, audio_data.duration_sec);
    printf("Will loop musical content, letting delay tail ring through naturally\n");
    
    audio_loaded = true;
    return true;
}

// Get RMS level for a specific frame (with looping)
float get_audio_rms_for_frame(int frame) {
    if (!audio_loaded || audio_data.num_frames == 0) {
        return 0.0f;
    }
    
    // Calculate how many frames represent the musical content
    float musical_duration = (float)musical_content_samples / audio_data.sample_rate / audio_data.channels;
    int musical_frames = (int)(musical_duration * VIS_FPS);
    
    if (musical_frames > 0 && musical_frames < (int)audio_data.num_frames) {
        // Loop only the musical content frames
        int looped_frame = frame % musical_frames;
        return audio_data.rms_levels[looped_frame];
    } else {
        // Fallback to full audio loop
        int looped_frame = frame % audio_data.num_frames;
        return audio_data.rms_levels[looped_frame];
    }
}

// Get current audio time for frame
float get_audio_time_for_frame(int frame) {
    if (!audio_loaded) return 0.0f;
    return frame / (float)VIS_FPS;
}

// Get total duration
float get_audio_duration(void) {
    return audio_loaded ? audio_data.duration_sec : 0.0f;
}

// Get BPM
float get_audio_bpm(void) {
    return audio_loaded ? audio_data.bpm : 120.0f;
}

// Check if we've reached the end of the audio
bool is_audio_finished(int frame) {
    if (!audio_loaded) return false;
    return frame >= (int)audio_data.num_frames;
}

// Get max RMS for normalization
float get_max_rms(void) {
    if (!audio_loaded) return 1.0f;
    
    float max_rms = 0.0f;
    for (uint32_t i = 0; i < audio_data.num_frames; i++) {
        if (audio_data.rms_levels[i] > max_rms) {
            max_rms = audio_data.rms_levels[i];
        }
    }
    return max_rms > 0.0f ? max_rms : 1.0f;
}

// Start audio playback
void start_audio_playback(void) {
    if (audio_device != 0) {
        audio_position = 0; // Reset to beginning
        SDL_PauseAudioDevice(audio_device, 0); // Start playback
        printf("Audio playback started\n");
    }
}

// Stop audio playback
void stop_audio_playback(void) {
    if (audio_device != 0) {
        SDL_PauseAudioDevice(audio_device, 1); // Pause playback
        printf("Audio playback stopped\n");
    }
}

// Get current playback position in seconds
float get_playback_position(void) {
    if (!audio_loaded || audio_data.sample_rate == 0) return 0.0f;
    return (float)audio_position / audio_data.channels / audio_data.sample_rate;
}

// Cleanup audio data
void cleanup_audio_data(void) {
    if (audio_device != 0) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
    
    if (audio_loaded) {
        free(audio_data.samples);
        free(audio_data.rms_levels);
        memset(&audio_data, 0, sizeof(audio_data));
        audio_loaded = false;
    }
}

// Print audio analysis info
void print_audio_info(void) {
    if (!audio_loaded) {
        printf("No audio loaded\n");
        return;
    }
    
    printf("=== Audio Analysis ===\n");
    printf("Duration: %.2f seconds\n", audio_data.duration_sec);
    printf("Sample Rate: %d Hz\n", audio_data.sample_rate);
    printf("Channels: %d\n", audio_data.channels);
    printf("Video Frames: %d\n", audio_data.num_frames);
    printf("Max RMS: %.3f\n", get_max_rms());
    printf("BPM: %.1f\n", audio_data.bpm);
    printf("=====================\n");
}
