# ARM64 Assembly Concurrent Audio Processing Guide

## Overview

This document captures critical findings and solutions for implementing multiple concurrent audio voices in ARM64 assembly, based on real debugging experience with the NotDeafBeef audio engine project.

## 🎯 Project Context

**Goal**: Implement concurrent kick, snare, hat, and melody voice processing in pure ARM64 assembly
**Challenge**: Memory corruption when multiple voice assembly functions execute together
**Success**: Individual voice functions work perfectly; multi-voice combinations cause segfaults

## 🔍 Root Cause Analysis: Buffer Pointer Preservation Bug

### The Problem

ARM64 assembly voice functions using libm calls (expf, sinf) experienced memory corruption due to incorrect assumptions about register preservation across library calls.

**Symptoms:**
- Individual voice assemblies: ✅ Work perfectly
- Multiple voice combinations: ❌ Segmentation faults
- Intermittent failures depending on memory layout

### Technical Root Cause

**Frame Layout Collision:**
```armasm
// INCORRECT (overlapping with vector registers)
stp x29, x30, [sp, #-240]!    // 240-byte frame
stp q14, q15, [sp, #0xd0]      // Vector regs at offset 208-223
str x23, [x29, #216]           // ❌ OVERLAPS with q14/q15!
str x24, [x29, #220]           // ❌ OVERLAPS with q14/q15!
```

**Buffer Pointer Corruption:**
- Voice functions load L/R buffer pointers into x23/x24
- libm calls (expf/sinf) corrupt these registers despite being "callee-saved"
- Corrupted pointers cause memory writes to invalid addresses
- Result: Segmentation faults during audio processing

## 🛠️ The Complete Fix: Buffer Pointer Preservation Pattern

### Frame Size Expansion
```armasm
// BEFORE (unsafe)
stp x29, x30, [sp, #-240]!    // 240-byte frame

// AFTER (safe)  
stp x29, x30, [sp, #-256]!    // 256-byte frame (16 extra bytes)
```

### Safe Offset Usage
```armasm
// BEFORE (unsafe - overlaps with vector registers)
str x23, [x29, #216]   // ❌ Collides with q14/q15 storage
str x24, [x29, #220]   // ❌ Collides with q14/q15 storage

// AFTER (safe - beyond all other data)
str x23, [x29, #240]   // ✅ Safe offset beyond vector registers
str x24, [x29, #248]   // ✅ Safe offset beyond vector registers
```

### Complete Implementation Pattern

**Around EVERY libm call (expf, sinf, etc.):**

```armasm
// BEFORE libm call
// BUFFER POINTER PRESERVATION: Save x23/x24 to safe slots before libm call
str x23, [x29, #240]   // Save L pointer to safe offset
str x24, [x29, #248]   // Save R pointer to safe offset
sub sp, sp, #112       // Create scratch frame for libm preservation
stp x0, x1, [sp]       // Save function parameters
stp x2, x3, [sp, #8]   // Save more parameters
// ... other register saves ...
bl _expf               // Call libm function

// AFTER libm call
// ... restore other registers ...
ldp x2, x3, [sp, #8]   // Restore function parameters
ldp x0, x1, [sp]       // Restore function parameters
add sp, sp, #112       // Restore stack pointer
// BUFFER POINTER PRESERVATION: Restore x23/x24 from safe slots after libm call
ldr x23, [x29, #240]   // Restore L pointer from safe offset
ldr x24, [x29, #248]   // Restore R pointer from safe offset
```

**Update Epilogue:**
```armasm
// BEFORE
ldp x29, x30, [sp], #240

// AFTER
ldp x29, x30, [sp], #256
```

## ✅ Implementation Results

### Individual Voice Success
Applied the buffer pointer preservation fix to all four voice functions:

- **kick.s**: ✅ Perfect audio generation, no corruption
- **snare.s**: ✅ Perfect audio generation, no corruption  
- **hat.s**: ✅ Perfect audio generation, no corruption
- **melody.s**: ✅ Perfect audio generation, no corruption

**Test Commands:**
```bash
# Each individual voice works perfectly
make segment USE_ASM=1 VOICE_ASM="KICK_ASM"     && ./bin/segment 0xCAFEBABE
make segment USE_ASM=1 VOICE_ASM="SNARE_ASM"    && ./bin/segment 0xCAFEBABE  
make segment USE_ASM=1 VOICE_ASM="HAT_ASM"      && ./bin/segment 0xCAFEBABE
make segment USE_ASM=1 VOICE_ASM="MELODY_ASM"   && ./bin/segment 0xCAFEBABE
```

### Multi-Voice Challenge
**Current Status:** Individual voices work; combinations still fail
```bash
# This still segfaults despite individual voice fixes
make segment USE_ASM=1 VOICE_ASM="KICK_ASM SNARE_ASM"
```

## 🔬 ARM64 Concurrent Audio Processing Considerations

### Memory Ordering Challenges

ARM64 has a **weak memory model** requiring explicit barriers for concurrent operations:

```armasm
// Memory barriers for concurrent audio processing
dmb ish    // Data Memory Barrier - Inner Shareable domain  
dsb ish    // Data Synchronization Barrier
isb        // Instruction Synchronization Barrier
```

### Stack Frame Interactions

When multiple voice functions execute:
- Each creates its own stack frame
- libm calls may affect global state
- Shared memory access patterns may conflict
- Vector register usage may overlap

### Common Concurrency Issues

1. **Shared State Corruption**
   - Global variables modified by multiple voices
   - Shared buffer access without proper synchronization

2. **Stack Corruption**
   - Frame pointer misalignment between voice functions
   - Overlapping scratch space usage

3. **Register Conflicts**
   - Vector register state not properly preserved
   - Floating-point register corruption across calls

## 🎯 Next Steps for Multi-Voice Assembly

### Systematic Debugging Approach

1. **Test Pair Combinations**
   ```bash
   # Test each pair to isolate problematic combinations
   make segment USE_ASM=1 VOICE_ASM="KICK_ASM SNARE_ASM"
   make segment USE_ASM=1 VOICE_ASM="KICK_ASM HAT_ASM"  
   make segment USE_ASM=1 VOICE_ASM="KICK_ASM MELODY_ASM"
   make segment USE_ASM=1 VOICE_ASM="SNARE_ASM HAT_ASM"
   # etc.
   ```

2. **LLDB Multi-Voice Analysis**
   ```bash
   # Set watchpoints on all voice buffer pointers
   lldb bin/segment
   watchpoint set expression -s 8 -- &kick_L_buffer
   watchpoint set expression -s 8 -- &snare_L_buffer
   # Identify which voice corrupts which buffer
   ```

3. **Memory Layout Analysis**
   - Check for overlapping buffer allocations
   - Verify stack frame alignment across voice functions
   - Analyze shared global state access patterns

### Potential Solutions

1. **Enhanced Memory Barriers**
   - Add explicit memory barriers between voice calls
   - Ensure memory coherency for shared audio buffers

2. **Isolated Frame Design**
   - Increase frame separation between voice functions
   - Use completely isolated scratch spaces

3. **Global State Protection**
   - Identify and protect shared global variables
   - Implement thread-safe access patterns

## 📚 References and Learning Resources

- ARM Architecture Reference Manual (ARM64)
- ARM64 ABI specifications for register preservation
- Audio processing concurrency patterns
- Memory ordering and synchronization in weak memory models

## 🏆 Success Metrics

- ✅ **Individual Voice Assembly**: COMPLETE SUCCESS
- ⚠️ **Multi-Voice Assembly**: In progress (individual fixes proven)
- 🎯 **Final Goal**: All four voices running together in pure assembly

---

*This document reflects the current state of ARM64 concurrent audio assembly development in the NotDeafBeef project, capturing both successful solutions and ongoing challenges.* 