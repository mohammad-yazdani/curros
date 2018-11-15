# curros

## Toolchain

`clang -v:
LLVM (http://llvm.org/):
  LLVM version 7.0.0
  Optimized build.
  Default target: x86_64-apple-darwin18.2.0
  Host CPU: broadwell`

### Generic compile command
`clang --target=x86_64-none-none-elf -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c <C source> -o <Object destination>`
clang --target=x86_64-pc-none-elf -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c kmain.c -o build/kmain.o
