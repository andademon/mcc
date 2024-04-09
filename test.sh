#!/bin/bash

riscv64-unknown-linux-gnu-gcc -static -g test.s -o test.elf

qemu-riscv64 test.elf