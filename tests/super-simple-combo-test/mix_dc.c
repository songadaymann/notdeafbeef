// tests/mix_two_dc.c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern void test_dc_process(float *L,float *R,uint32_t n);
extern void test_dc_neg_process(float *L,float *R,uint32_t n);

int main(void)
{
    const uint32_t N = 44100;           /* 1-second buffer */
    float *L = calloc(N,sizeof(float));
    float *R = calloc(N,sizeof(float));

    test_dc_process(L,R,N);             /* +0.1 */
    test_dc_neg_process(L,R,N);         /* −0.1  → should cancel */

    /* RMS should be ~0 */
    double sum=0;
    for(uint32_t i=0;i<N;i++) sum += L[i]*L[i] + R[i]*R[i];
    printf("RMS = %.6f\n", sqrt(sum/(N*2)));

    free(L); free(R);
    return 0;
}