#!/bin/bash

# LLDB script to debug delay struct corruption
# Based on roadmap section: "place an LLDB watch-point on delay.idx to catch the first instruction that overwrites the struct"

lldb ./bin/segment << 'EOF'
# Set breakpoint right after delay_init to get addresses
br set -f generator.c -l 113
run 0xCAFEBABE

# Set watchpoints on delay struct members
# Note: addresses will be different each run, but pattern should be similar
# We'll set watchpoints on both size and idx since both get corrupted
watchpoint set expression -w write -- (uint32_t*)$rdi+8
watchpoint set expression -w write -- (uint32_t*)$rdi+12

# Continue and catch the corruption
continue

# When watchpoint hits, show backtrace and register state
bt
register read

# Exit
quit
EOF 