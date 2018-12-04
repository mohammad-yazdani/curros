cd ~/os-dev/curros

mkdir -p user
touch user/user.zip
echo "UNZIPPING USER CODE"
unzip -o user/user.zip

echo "Building USER APPS"
cd ./user 
make
cd ..


usuc="USER APPS built with [CODE $?]"

echo "Building KERNEL"
make
echo ""
echo "======================================================="
echo "BUILD SUMMARY:"
echo "KERNEL built with [CODE $?]"
echo ${usuc}
echo "======================================================="
echo ""

zip -r out.zip ./out

