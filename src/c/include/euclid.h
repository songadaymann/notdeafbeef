#ifndef EUCLID_H
#define EUCLID_H

#include <stddef.h>
#include <stdint.h>

/* Fill `out[steps]` with 0/1 values representing a Bjorklund Euclidean rhythm.
 * `pulses` may be 0..steps.
 */
void euclid_pattern(int pulses, int steps, uint8_t *out);

#endif /* EUCLID_H */ 