#ifndef FM_PRESETS_H
#define FM_PRESETS_H

#include <stddef.h>

typedef struct {
    float32_t ratio;
    float32_t index;
    float32_t decay;
    float32_t amp;
} fm_params_t;

/* Mid-range FM timbres (used by python: fm_bells, fm_calm, fm_quantum, fm_pluck) */
extern const fm_params_t FM_PRESET_BELLS;
extern const fm_params_t FM_PRESET_CALM;
extern const fm_params_t FM_PRESET_QUANTUM;
extern const fm_params_t FM_PRESET_PLUCK;

/* Bass FM profiles (python bass_profiles) */
extern const fm_params_t FM_BASS_DEFAULT;
extern const fm_params_t FM_BASS_QUANTUM;
extern const fm_params_t FM_BASS_PLUCKY;

#endif /* FM_PRESETS_H */ 