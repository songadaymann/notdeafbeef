// src/asm/active/test_dc.s
    .text
    .align 2
    .globl _test_dc_process
// void test_dc_process(float *L, float *R, uint32_t n)
_test_dc_process:
    cbz w2, .Ldone           // n==0 → return
    mov  w3, #0xcccd        // low 16 bits of 0.1f
    movk w3, #0x3dcc, lsl #16
    fmov s0, w3              // 0.1f
.Lloop:
    str s0, [x0], #4         // *L++ = 0.1
    str s0, [x1], #4         // *R++ = 0.1
    subs w2, w2, #1
    b.ne .Lloop
.Ldone:
    ret