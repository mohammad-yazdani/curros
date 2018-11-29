
scp ./src/* carbe:~/os-dev/curros/src
scp ./inc/* carbe:~/os-dev/curros/inc
scp ./makefile carbe:~/os-dev/curros/

ssh carbe 'bash -s' < ./compile_remote.sh

mkdir -p out
scp carbe:~/os-dev/curros/out/kernel.elf ./out
scp carbe:~/os-dev/curros/out/curros.iso ./out
