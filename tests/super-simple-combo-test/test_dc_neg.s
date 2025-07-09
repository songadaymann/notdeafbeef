// src/asm/active/test_dc_neg.s
    .text
    .align 2
    .globl _test_dc_neg_process
_test_dc_neg_process:
    cbz w2, .Ldone
    mov  w3, #0xcccd        // low16 of 0.1f (sign later)
    movk w3, #0xbdcc, lsl #16
    fmov s0, w3              // -0.1f
.Lloop:
    str s0, [x0], #4
    str s0, [x1], #4
    subs w2, w2, #1
    b.ne .Lloop
.Ldone:
    ret