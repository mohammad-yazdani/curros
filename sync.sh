
zip -r user/user.zip user

scp ./user/user.zip carbe:~/os-dev/curros/user/user.zip
scp ./src/* carbe:~/os-dev/curros/src
scp ./inc/* carbe:~/os-dev/curros/inc
scp ./storage carbe:~/os-dev/curros/
scp ./makefile carbe:~/os-dev/curros/
scp ./grub.cfg carbe:~/os-dev/curros/

ssh carbe 'bash -s' < ./compile_remote.sh

mkdir -p out
scp carbe:~/os-dev/curros/out/kernel.elf ./out
scp carbe:~/os-dev/curros/out/curros.iso ./out

