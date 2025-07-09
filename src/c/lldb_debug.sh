#!/bin/bash
# LLDB script to debug delay struct corruption
lldb ./bin/segment << 'LLDB_EOF'
br set -f generator.c -l 113
run 0xCAFEBABE
# Get the actual address of delay struct from the debug output
# Then set watchpoint on delay.size and delay.idx 
p &((generator_t*)0)->delay.size
p &((generator_t*)0)->delay.idx
continue
quit
LLDB_EOF
