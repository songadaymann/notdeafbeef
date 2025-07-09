#include "limiter.h"

void limiter_process(limiter_t *l, float32_t *L, float32_t *R, uint32_t n)
{
    float32_t env = l->envelope;
    float32_t att = l->attack_coeff;
    float32_t rel = l->release_coeff;
    float32_t thresh = l->threshold;

    for(uint32_t i = 0; i < n; ++i){
        // detect instantaneous stereo peak
        float32_t peak = fmaxf(fabsf(L[i]), fabsf(R[i]));

        // simple envelope follower
        env = (peak > env) ? peak + att * (env - peak)
                           : peak + rel * (env - peak);

        // HARD LIMITER: if env exceeds threshold, scale sample so env == thresh
        float32_t gain = 1.0f;
        if(env > thresh){
            gain = thresh / env;
            L[i] *= gain;
            R[i] *= gain;
            env = thresh; // clamp envelope so release starts here
        }
    }
    l->envelope = env;
} 