cd ~/os-dev/curros

mkdir -p user
touch user/user.zip
echo "UNZIPPING USER CODE"
unzip -o user/user.zip

echo "Building USER APPS"
cd ./user 
make
cp ./out/hello_hell.elf ../storage
echo "USER APPS built with code {$?}"

cd ..
echo "Building KERNEL"
make
echo "KERNEL built with code {$?}"





