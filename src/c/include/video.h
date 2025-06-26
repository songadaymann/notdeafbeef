#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>
#include <stdbool.h>

/* Simple SDL2 + OpenGL video facade used by the visualiser.
 * This is intentionally minimal for Stage-1 scaffolding.
 */

/* Initialise window & renderer.
 * width/height → window size (pixels)
 * fps          → target frame-rate (logical). 0 = uncapped.
 * vsync        → true  = let SDL use VSYNC; false = immediate.
 * Returns 0 on success, non-zero on failure.
 */
int  video_init(int width, int height, int fps, bool vsync);

/* Begin a new frame. Handles frame pacing, event polling, clears the screen.
 * Returns false when the user has requested to quit (e.g. window close).
 */
bool video_frame_begin(void);

/* End the frame: present back-buffer. */
void video_frame_end(void);

/* Accessors for software framebuffer */
uint32_t* video_get_framebuffer(void);
int video_get_width(void);
int video_get_height(void);

/* Shutdown & free resources. */
void video_shutdown(void);

#endif /* VIDEO_H */ 