#!/bin/bash
echo  "[0%] Updating apt-get..."
sudo apt-get update 
echo  "[0%] Updating apt-get... Done!"
echo  "[5%] Upgrading apt-get..."
sudo apt-get upgrade --assume-yes --force-yes
echo  "[5%] Upgrading apt-get... Done!"
echo  "[10%] Finished."

install_package () {
    REQUIRED_PKG="$1"
    PKG_OK=$(dpkg-query -W --showformat='${Status}\n' $REQUIRED_PKG|grep "install ok installed")
    if [ "" = "$PKG_OK" ]; then
    sudo apt-get --assume-yes --force-yes -y install $REQUIRED_PKG 
    else 
    echo "Already installed."
    fi
}

uninstall_package () {
    REQUIRED_PKG="$1"
    PKG_OK=$(dpkg-query -W --showformat='${Status}\n' $REQUIRED_PKG|grep "install ok installed")
    if [ "install ok installed" = "$PKG_OK" ]; then
    sudo apt remove --purge --auto-remove --assume-yes --force-yes -y $REQUIRED_PKG
    fi
    
    # clean remaining locks
    sleep 1
    sudo killall apt apt-get 2> /dev/null
    sudo rm /var/lib/apt/lists/lock 2> /dev/null
    sudo rm /var/cache/apt/archives/lock 2> /dev/null
    sudo rm /var/lib/dpkg/lock 2> /dev/null
    sudo rm /var/lib/dpkg/lock-frontend 2> /dev/null
}

echo  ""
echo  "[10%] Installing dependency: build-essential... "

install_package build-essential

echo  ""
echo  "[11%] Installing dependency: build-essential... Done!"

echo  ""
echo  "[11%] Installing dependency: cmake... "

version=3.14
build=1
wget https://cmake.org/files/v$version/cmake-$version.$build.tar.gz
tar -xzvf cmake-$version.$build.tar.gz
cd cmake-$version.$build
./bootstrap --prefix=/usr && make -j$(nproc) && sudo make install
cd -

echo  ""
echo  "[11%] Installing dependency: cmake... Done!"

echo  ""
echo  "[12%] Installing dependency: git... "

install_package git

echo  ""
echo  "[12%] Installing dependency: git... Done!"


echo  ""
echo  "[12%] Installing dependency: python3... "

install_package python3

echo  ""
echo  "[13%] Installing dependency: python3... Done!"

echo  ""
echo  "[13%] Installing dependency: Boost 1.71.0... "

wget https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.gz
tar -xf boost_1_71_0.tar.gz

cd boost_1_71_0
./bootstrap.sh --prefix=/usr --with-python=python3 &&
sudo ./b2 stage -j$(nproc) threading=multi link=shared --with-regex --with-test --with-filesystem --with-date_time --with-random --with-system --with-thread --with-program_options --with-chrono --with-fiber --with-log --with-context --with-math && sudo ./b2 install --prefix=/usr
cd -

echo  ""
echo  "[13%] Installing dependency: Boost 1.71.0... Done!"

echo  ""
echo  "[15%] Installing dependency: libtool... "

install_package libtool

echo  ""
echo  "[15%] Installing dependency: libtool... Done!"

echo  ""
echo  "[16%] Installing dependency: bsdmainutils... "

install_package bsdmainutils

echo  ""
echo  "[16%] Installing dependency: bsdmainutils... Done!"

echo  ""
echo  "[17%] Installing dependency: autotools-dev... "

install_package autotools-dev

echo  ""
echo  "[17%] Installing dependency: autotools-dev... Done!"

echo  ""
echo  "[18%] Installing dependency: autoconf... "

install_package autoconf

echo  ""
echo  "[18%] Installing dependency: autoconf... Done!"

echo  ""
echo  "[19%] Installing dependency: pkg-config... "

install_package pkg-config

echo  ""
echo  "[19%] Installing dependency: pkg-config... Done!"

echo  ""
echo  "[20%] Installing dependency: automake... "

install_package automake

echo  ""
echo  "[20%] Installing dependency: automake... Done!"

echo  ""
echo  "[22%] Installing dependency: libminiupnpc-dev... "

install_package libminiupnpc-dev

echo  ""
echo  "[22%] Installing dependency: libminiupnpc-dev... Done!"

echo  ""
echo  "[23%] Installing dependency: miniupnpc... "

install_package miniupnpd

echo  ""
echo  "[23%] Installing dependency: libminiupnpc-dev... Done!"

echo  ""
echo  "[23%] Installing dependency: libzmq3-dev... "

install_package libzmq3-dev

echo  ""
echo  "[23%] Installing dependency: libzmq3-dev... Done!"

echo  ""
echo  "[24%] Installing dependency: librocksdb-dev... "

install_package librocksdb-dev

echo  ""
echo  "[24%] Installing dependency: librocksdb-dev... Done!"

echo  ""
echo  "[25%] Installing dependency: libssl-dev... "

install_package libssl-dev

echo  ""
echo  "[25%] Installing dependency: libssl-dev... Done!"

echo  ""
echo  "[26%] Installing dependency: libgmp-dev... "

install_package libgmp-dev

echo  ""
echo  "[26%] Installing dependency: libgmp-dev... Done!"

echo  ""
echo  "[27%] Installing dependency: libevent-dev... "

install_package libevent-dev

echo  ""
echo  "[27%] Installing dependency: libevent-dev... Done!"

echo  ""
echo  "[28%] Installing dependency: libjsonrpccpp-dev... "

install_package libjsonrpccpp-dev

echo  ""
echo  "[28%] Installing dependency: libjsonrpccpp-dev... Done!"

echo  ""
echo  "[29%] Installing dependency: libsnappy-dev... "

install_package libsnappy-dev

echo  ""
echo  "Done!"

echo  ""
echo  "[30%] Installing dependency: libbenchmark-dev... "

install_package libbenchmark-dev

echo  ""
echo  "[30%] Installing dependency: libbenchmark-dev... Done!"

echo  ""
echo  "[31%] Installing dependency: libgtest-dev... "

install_package libgtest-dev

echo  ""
echo  "[31%] Installing dependency: libgtest-dev... Done!"
echo  ""

echo  "[32%] Configuring GTest... "

cd /usr/src/googletest
sudo cmake . && sudo cmake --build . --target install && cd -

echo  ""
echo  "[32%] Configuring GTest... Done!"

echo  ""
echo  "[32%] Checking Berkeley DB... "

uninstall_package libdb-dev

uninstall_package libdb++-dev

echo  ""
echo  "[32%] Checking Berkeley DB... Done!"

echo  ""
echo  "[33%] Installing dependency: libprotobuf-dev... "

install_package libprotobuf-dev

echo  ""
echo  "[33%] Installing dependency: libprotobuf-dev... Done!"

echo  ""
echo  "[34%] Installing dependency: protobuf-compiler... "

install_package protobuf-compiler

echo  ""
echo  "[34%] Installing dependency: protobuf-compiler... Done!"

echo  ""
echo  "[35%] Installing dependency: libqrencode-dev... "

install_package libqrencode-dev

echo  ""
echo  "[35%] Installing dependency: libqrencode-dev... Done!"

echo  ""
echo  "[36%] Installing dependency: libpng-dev... "

install_package libpng-dev

echo  ""
echo  "[36%] Installing dependency: libpng-dev... Done!"

echo  ""
echo  "[39%] Installing QT Components. "

echo  ""
echo  "[40%] Installing QT dependency: libqt5gui5... "

install_package libqt5gui5

echo  ""
echo  "[40%] Installing QT dependency: libqt5gui5... Done!"

echo  ""
echo  "[41%] Installing QT dependency: libqt5core5a... "

install_package libqt5core5a

echo  ""
echo  "[41%] Installing QT dependency: libqt5core5a... Done!"

echo  ""
echo  "[42%] Installing QT dependency: libqt5dbus5... "

install_package libqt5dbus5

echo  ""
echo  "[42%] Installing QT dependency: libqt5dbus5... Done!"

echo  ""
echo  "[43%] Installing QT dependency: qttools5-dev... "

install_package qttools5-dev

echo  ""
echo  "[43%] Installing QT dependency: qttools5-dev... Done!"

echo  ""
echo  "[44%] Installing QT dependency: qttools5-dev-tools... "

install_package qttools5-dev-tools

echo  ""
echo  "[44%] Installing QT dependency: qttools5-dev-tools... Done!"

echo  ""
echo  "[45%] Installing QT dependency: libqt5svg5-dev... "

install_package libqt5svg5-dev

echo  ""
echo  "[45%] Installing QT dependency: libqt5svg5-dev... Done!"

echo  ""
echo  "[46%] Installing QT dependency: libqt5charts5-dev... "

install_package libqt5charts5-dev

echo  ""
echo  "[46%] Installing QT dependency: libqt5charts5-dev... Done!"

echo  ""
echo  "[47%] All QT Components has been installed. "

echo  ""
echo  "[50%] Checking is folder the git repository... "
if [ -d .git ]; then
echo -ne  "yes"
    if [ "$1" = "update" ]
    then
    echo  ""
    echo  "[50%] Updating current version of the BTCU... "
    git checkout master 

    if [ -s "versions.txt" ]
        then
            file="versions.txt"
            l=""

            while IFS= read line
            do
            l=$line
            done <"$file"

            my_var="$( cut -d ' ' -f 2 <<< "$l" )";
            echo  ""
            echo  "[51%] Working branch: release_$my_var"
            git checkout "release_$my_var"
        else
            echo  ""
            echo  "[51%] Working branch: master"
        fi

    git pull
    echo  ""
    echo  "[50%] Updating current version of the BTCU... Done!"
    echo  ""
    
    else 
    
    echo  ""
    echo  "[50%] Updating current version of the BTCU"
    
    fi
else
    echo -ne  "no"
    echo  ""
    echo  "[50%] Downloading latest version of the BTCU... "
    git clone https://github.com/bitcoin-ultimatum/orion
    cd orion
    echo  ""
    echo  "[50%] Downloading latest version of the BTCU... Done!"
    echo  ""
fi;

echo  ""
echo  "[60%] Installing Berkeley DB... "


if [ -f /opt/lib/libdb-18.1.a ]
then
    echo  "[60%] Berkeley DB is already installed."
else
    # Since as of 5th March 2020 the Oracle moved Barkeley DB 
    # to login-protected tarball for 18.1.32 version 
    # we added the dependency as a static file included in the repository.
    # You can check the details in depends/packages/static/berkeley-db-18.1.32/README.MD

    tar zxvf depends/packages/static/berkeley-db-18.1.32/berkeley-db-18.1.32.tar.gz -C ./
    cd  db-18.1.32/build_unix
    ../dist/configure --enable-cxx --disable-shared --disable-replication --with-pic --prefix=/opt && make && sudo make install
    cd -
fi

echo  ""
echo  "[60%] Installing Berkeley DB... Done!"

echo  ""
echo  "[65%] Running CMake configuring... "

cmake -G "CodeBlocks - Unix Makefiles" .

echo  ""
echo  "[65%] Running CMake configuring... Done!"

echo  ""
echo  "[75%] Building BTCU... "

make

echo  ""
echo  "[75%] Building BTCU... Done!"

echo  ""
echo  "[100%] Build is completed!"

echo  ""
echo  ""
echo  ""

echo  "=========================================================="
echo  "The built binaries was placed in ./bin folder"
echo  "For start daemon please run:"
echo  "./bin/btcud -daemon"
echo  "Outputs a list of command-line options:"
echo  "./bin/btcu-cli --help"
echo  "Outputs a list of RPC commands when the daemon is running:"
echo  "./bin/btcu-cli help"
echo  "Start GUI:"
echo  "./src/qt/btcu-qt"
echo  "=========================================================="
