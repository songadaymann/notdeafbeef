#ifndef OSC_H
#define OSC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Basic phase-accumulator oscillator helpers. */

typedef struct {
    float32_t phase; /* in radians */
} osc_t;

static inline void osc_reset(osc_t *o) { o->phase = 0.0f; }

/* Render `n` mono samples of a sine wave at `freq` Hz into `out`.
 * The oscillator keeps its phase between calls.
 */
void osc_sine_block(osc_t *o, float32_t *out, uint32_t n, float32_t freq, float32_t sr);

/* Render `n` mono samples of a saw wave at `freq` Hz into `out`.
 * The oscillator keeps its phase between calls.
 */
void osc_saw_block(osc_t *o, float32_t *out, uint32_t n, float32_t freq, float32_t sr);

/* Render `n` mono samples of a square wave at `freq` Hz into `out`.
 * The oscillator keeps its phase between calls.
 */
void osc_square_block(osc_t *o, float32_t *out, uint32_t n, float32_t freq, float32_t sr);

/* Render `n` mono samples of a triangle wave at `freq` Hz into `out`.
 * The oscillator keeps its phase between calls.
 */
void osc_triangle_block(osc_t *o, float32_t *out, uint32_t n, float32_t freq, float32_t sr);

#ifdef __cplusplus
}
#endif

#endif /* OSC_H */ 