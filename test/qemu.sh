#!/bin/bash
qemu-system-x86_64 -m 128 -monitor stdio -cdrom out/curros.iso -s -S -d cpu_reset


