#!/bin/bash

if [ "$1" = "update" ]

then
echo "Downloadind latest version BTCU"
git checkout master 

if [ -s "versions.txt" ]
then
file="versions.txt"
l=""
while IFS= read line
do
l=$line
done <"$file"

#echo $l | cut -d ' ' -f2
my_var="$( cut -d ' ' -f 2 <<< "$l" )";

echo 
git checkout "release_$my_var"
git pull
else
git checkout master
git pull

fi

fi
echo "Downloading latest version"
git checkout master
git pull
echo "Install BTCU dependencies"

if [ -f /opt/lib/libdb-18.1.a ]
then
echo "BerkeleyDB installed"
else
sudo apt-get install libboost-all-dev

sudo apt-get purge libdb4.8-dev libdb4.8++-dev

# Since as of 5th March 2020 the Oracle moved Barkeley DB 
# to login-protected tarball for 18.1.32 version 
# we added the dependency as a static file included in the repository.
# You can check the details in depends/packages/static/berkeley-db-18.1.32/README.MD

tar zxvf depends/packages/static/berkeley-db-18.1.32/berkeley-db-18.1.32.tar.gz -C ./
cd  db-18.1.32/build_unix
../dist/configure --enable-cxx --disable-shared --disable-replication --with-pic --prefix=/opt
make

sudo make install

cd ../..
fi
ls
sudo apt-get install libminiupnpc-dev

if [ -f /usr/local/lib/libzmq.so ]
then
echo "ZMQ installed"
else
#ZMQ dependencies (provides ZMQ API):

    wget https://github.com/zeromq/libzmq/releases/download/v4.2.2/zeromq-4.2.2.tar.gz

    # Unpack tarball package
    tar xvzf zeromq-4.2.2.tar.gz

    # Install dependency
    sudo apt-get update && \
    sudo apt-get install -y libtool pkg-config build-essential autoconf automake uuid-dev

    # Create make file
    cd zeromq-4.2.2
    ./configure

    # Build and install(root permission only)
    sudo make install

    # Install zeromq driver on linux
    sudo ldconfig

    # Check installed
    ldconfig -p | grep zmq

    cd ..
    # Expected
    ############################################################
    # libzmq.so.5 (libc6,x86-64) => /usr/local/lib/libzmq.so.5
    # libzmq.so (libc6,x86-64) => /usr/local/lib/libzmq.so
    ############################################################
fi

ls
sudo apt-get install build-essential libtool bsdmainutils autotools-dev autoconf pkg-config automake python3

#Cmake installing:

#(minimum required version 3.10)

sudo apt-get install cmake

#For Ethereum VM smart contracts:

sudo apt-get install librocksdb-dev libjsoncpp-dev

#Now, you can either build from self-compiled [depends](/depends/README.md) or install the required dependencies:

sudo apt-get install libssl-dev libgmp-dev libevent-dev libboost-all-dev

#Minimum required version 1.65

sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 libqt5svg5-dev libqt5charts5-dev qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev libpng-dev

git clone https://github.com/fukuchi/libqrencode.git
cd libqrencode
cmake CMakeLists.txt
make
sudo make install

cd ..

ls

cd src/univalue/
./autogen.sh
./configure

make

cd ../secp256k1/
./autogen.sh
./configure --disable-shared --with-pic --with-bignum=no --enable-module-recovery --enable-module-ecdh --enable-experimental --disable-jni

make
cd ../../

./autogen.sh
./configure BDB_LIBS="-L/opt/lib -ldb_cxx-18.1" BDB_CFLAGS="-I/opt/include"
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - Unix Makefiles" ..
make -j4 btcud
make -j4 btcu-cli
make -j4 btcu-qt
cd ..
#make -j4 btcu-tx
