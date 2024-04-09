#! /bin/bash

riscv64-unknown-linux-gnu-gcc -static -g test.s -o test.elf

qemu-riscv64 -singlestep -g 1234 test.elf &

riscv64-unknown-linux-gnu-gdb test.elf -q -x ./gdbinit