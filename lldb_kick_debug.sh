#!/bin/bash

# LLDB script to find kick.s corruption of delay struct
lldb -o "target create bin/segment" \
     -o "br s -n generator_process" \
     -o "run 0xCAFEBABE" \
     -o "watchpoint set expression -s 4 -- 0x16d37b6b0" \
     -o "watchpoint set expression -s 4 -- 0x16d37b6b4" \
     -o "watchpoint command add 1 -o 'bt' -o 'disassemble -p'" \
     -o "watchpoint command add 2 -o 'bt' -o 'disassemble -p'" \
     -o "continue" \
     -o "quit" 