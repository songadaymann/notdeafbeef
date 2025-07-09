#!/bin/bash

# LLDB Crash Analysis for Multi-Voice Assembly Integration
# Focus: Get detailed crash information for the segfaulting segment binary

echo "🔍 === LLDB CRASH ANALYSIS FOR MULTI-VOICE INTEGRATION ==="
echo "Goal: Identify exact crash location and register corruption patterns"
echo ""

cd src/c

# Ensure we have a debug build
echo "Building with debug symbols..."
make clean
make segment USE_ASM=1 DEBUG=1

echo ""
echo "🚀 Running LLDB analysis..."

lldb ./bin/segment << 'EOF'
# Set up for comprehensive debugging
settings set target.process.stop-on-sharedlibrary-events false
settings set target.x86-disassembly-flavor intel

# Run until crash
run 0xDEBUG123

# Show crash details
echo "=== CRASH BACKTRACE ==="
bt

echo "=== REGISTER STATE ==="
register read

echo "=== CURRENT FRAME INFO ==="
frame info

echo "=== MEMORY AROUND STACK POINTER ==="
memory read --format hex --size 128 $rsp

echo "=== MEMORY AROUND FRAME POINTER ==="
memory read --format hex --size 128 $rbp

echo "=== DISASSEMBLY AT CRASH POINT ==="
disassemble --pc

echo "=== CURRENT FUNCTION DISASSEMBLY ==="
disassemble --frame

echo "=== THREAD INFO ==="
thread info

# Try to identify which voice function was executing
echo "=== LOOKING FOR VOICE FUNCTION CONTEXT ==="
frame select 0
image lookup --address $pc

# Show variables if available
echo "=== LOCAL VARIABLES ==="
frame variable

quit
EOF

echo ""
echo "✅ LLDB analysis complete!"
echo "📋 Look for patterns in:"
echo "   - Which assembly function was executing at crash"
echo "   - Register corruption (especially x27, x28, sp, fp)"
echo "   - Stack frame integrity"
echo "   - Memory access patterns" 