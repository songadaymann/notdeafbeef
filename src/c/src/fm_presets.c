#include "fm_presets.h"

const fm_params_t FM_PRESET_BELLS   = {3.5f, 4.0f, 8.0f, 0.25f};
const fm_params_t FM_PRESET_CALM    = {2.0f, 2.5f, 6.0f, 0.25f};
const fm_params_t FM_PRESET_QUANTUM = {1.5f, 3.0f, 6.0f, 0.25f};
/* fm_pluck uses high index that decays quickly; index stored as peak */
const fm_params_t FM_PRESET_PLUCK   = {1.0f, 6.0f, 8.0f, 0.25f};

const fm_params_t FM_BASS_DEFAULT = {2.0f, 5.0f, 4.0f, 0.4f};
const fm_params_t FM_BASS_QUANTUM = {1.5f, 8.0f, 8.0f, 0.45f};
const fm_params_t FM_BASS_PLUCKY  = {3.0f, 2.5f, 14.0f, 0.35f}; 