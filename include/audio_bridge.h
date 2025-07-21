#ifndef AUDIO_BRIDGE_H
#define AUDIO_BRIDGE_H

/*
 * Audio Bridge Interface
 * 
 * Minimal read-only interface to expose audio timing and RMS data
 * to the visual system without sharing any code.
 * 
 * Hard isolation principle: Audio and visuals synchronize via timecode only.
 */

/**
 * Get current position in audio segment (milliseconds)
 * @return Current audio time in milliseconds since segment start
 */
unsigned int get_audio_time_ms(void);

/**
 * Get pre-computed RMS level for visual sizing
 * @param frame_idx Frame index for RMS lookup
 * @return RMS level (0.0 to 1.0) for visual scaling
 */
float get_rms_level(int frame_idx);

/**
 * Check if audio system is currently active/playing
 * @return 1 if audio is active, 0 if stopped
 */
int is_audio_active(void);

/**
 * Get segment duration in milliseconds
 * @return Total segment duration in milliseconds
 */
unsigned int get_segment_duration_ms(void);

#endif /* AUDIO_BRIDGE_H */
