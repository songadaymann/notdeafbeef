#include "limiter.h"

void limiter_process(limiter_t *l, float32_t *L, float32_t *R, uint32_t n)
{
    float32_t env = l->envelope;
    float32_t att = l->attack_coeff;
    float32_t rel = l->release_coeff;
    float32_t thresh = l->threshold;
    float32_t knee_db = l->knee_width;

    for(uint32_t i=0;i<n;++i){
        // stereo peak detection
        float32_t peak = fmaxf(fabsf(L[i]), fabsf(R[i]));

        // envelope follower
        if(peak > env) env = peak + att * (env - peak);
        else           env = peak + rel * (env - peak);

        // gain reduction calculation (soft knee)
        float32_t overshoot_db = 20.0f * log10f(env / thresh);
        float32_t gain_reduction_db = 0.0f;
        if (overshoot_db > -knee_db/2.0f) {
            if (overshoot_db < knee_db/2.0f) {
                // inside knee
                gain_reduction_db = powf(overshoot_db + knee_db/2.0f, 2.0f) / (2.0f * knee_db);
            } else {
                // above knee (hard limiting)
                gain_reduction_db = overshoot_db;
            }
        }
        
        // convert gain reduction to linear scale and apply
        float32_t gain = powf(10.0f, -gain_reduction_db / 20.0f);
        if(gain < 1.0f){
            L[i] *= gain;
            R[i] *= gain;
        }
    }
    l->envelope = env;
} 