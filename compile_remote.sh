cd ~/os-dev/curros

mkdir -p user
touch user/user.zip
echo "UNZIPPING USER CODE"
unzip -o user/user.zip

echo "Building USER APPS"
cd ./user 
make
cp ./out/hello_hell.elf ../storage

usuc="USER APPS built with [CODE $?]"

cd ..
echo "Building KERNEL"
make
echo ""
echo "======================================================="
echo "BUILD SUMMARY:"
echo "KERNEL built with [CODE $?]"
echo ${usuc}
echo "======================================================="
echo ""

