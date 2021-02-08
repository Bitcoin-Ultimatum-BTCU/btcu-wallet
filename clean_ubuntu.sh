#!/bin/bash
echo "[0%] Cleaning all installed components..."
echo "[5%] Warning! The BTCU data directory with the wallet will remain untouched."
echo "[9%] Warning! Will be uninstalled followed libraries: "
echo "         -   cmake,"
echo "         -   git,"
echo "         -   libboost-all-dev,"
echo "         -   build-essential,"
echo "         -   libtool,"
echo "         -   autotools-dev,"
echo "         -   autoconf,"
echo "         -   pkg-config,"
echo "         -   automake,"
echo "         -   libminiupnpc-dev,"
echo "         -   libzmq3-dev,"
echo "         -   libssl-dev,"
echo "         -   libgmp-dev,"
echo "         -   libevent-dev,"
echo "         -   libjsonrpccpp-dev,"
echo "         -   libsnappy-dev,"
echo "         -   libgtest-dev,"
echo "         -   libprotobuf-dev,"
echo "         -   protobuf-compiler,"
echo "         -   libqrencode-dev,"
echo "         -   libpng-dev,"
echo "         -   libqt5gui5,"
echo "         -   libqt5core5a,"
echo "         -   libqt5dbus5,"
echo "         -   qttools5-dev,"
echo "         -   qttools5-dev-tools,"
echo "         -   libqt5svg5-dev,"
echo "         -   libqt5charts5-dev,"
echo "         -   Berkeley DB."
echo ""

# TODO: add prompt and option for silence agreement to keep dependencies: 
# - base (cmake, git, etc), 
# - boost, 
# - build tools, 
# - project dependecies, 
# - QT dependencies, 
# - OpenSSL, 
# - GMP 
# - and DB dependencies

uninstall_package () {
    REQUIRED_PKG="$1"
    PKG_OK=$(dpkg-query -W --showformat='${Status}\n' $REQUIRED_PKG|grep "install ok installed")
    if [ "install ok installed" = "$PKG_OK" ]; then
    sudo apt remove --purge --auto-remove -y --force-yes $REQUIRED_PKG
    fi
    
    # clean remaining locks
    sleep 1
    sudo killall apt apt-get 2> /dev/null
    sudo rm /var/lib/apt/lists/lock 2> /dev/null
    sudo rm /var/cache/apt/archives/lock 2> /dev/null
    sudo rm /var/lib/dpkg/lock 2> /dev/null
    sudo rm /var/lib/dpkg/lock-frontend 2> /dev/null
}

echo "[10%] Uninstalling dependency: cmake... "

uninstall_package cmake

echo  "Done!"

echo "[12%] Uninstalling dependency: git... "

uninstall_package git

echo  "Done!"

echo "[14%] Uninstalling dependency: libboost-all-dev... "

uninstall_package libboost-all-dev

echo  "Done!"

echo "[16%] Uninstalling dependency: build-essential... "

uninstall_package build-essential

echo  "Done!"

echo "[18%] Uninstalling dependency: libtool... "

uninstall_package libtool

echo  "Done!"

echo "[22%] Uninstalling dependency: autotools-dev... "

uninstall_package autotools-dev

echo  "Done!"

echo "[24%] Uninstalling dependency: autoconf... "

uninstall_package autoconf

echo  "Done!"

echo "[26%] Uninstalling dependency: pkg-config... "

uninstall_package pkg-config

echo  "Done!"

echo "[28%] Uninstalling dependency: automake... "

uninstall_package automake

echo  "Done!"

echo "[32%] Uninstalling dependency: libminiupnpc-dev... "

uninstall_package libminiupnpc-dev
uninstall_package miniupnpd

echo  "Done!"

echo "[34%] Uninstalling dependency: libzmq3-dev... "

uninstall_package libzmq3-dev

echo  "Done!"

echo "[36%] Uninstalling dependency: librocksdb-dev... "

uninstall_package librocksdb-dev

echo  "Done!"

echo "[38%] Uninstalling dependency: libssl-dev... "

uninstall_package libssl-dev

echo  "Done!"

echo "[40%] Uninstalling dependency: libgmp-dev... "

uninstall_package libgmp-dev

echo  "Done!"

echo "[42%] Uninstalling dependency: libevent-dev... "

uninstall_package libevent-dev

echo  "Done!"

echo "[44%] Uninstalling dependency: libjsonrpccpp-dev... "

uninstall_package libjsonrpccpp-dev

echo  "Done!"

echo "[46%] Uninstalling dependency: libsnappy-dev... "

uninstall_package libsnappy-dev

echo  "Done!"

echo "[48%] Uninstalling dependency: libbenchmark-dev... "

uninstall_package libbenchmark-dev

echo  "Done!"

echo "[50%] Uninstalling dependency: libgtest-dev... "

uninstall_package libgtest-dev

echo  "Done!"

echo "[52%] Uninstalling Berkeley DB... "

if [ -f /opt/lib/libdb-18.1.a ]
then
    cd  db-18.1.32/build_unix
    ../dist/configure --enable-cxx --disable-shared --disable-replication --with-pic --prefix=/opt
    make
    sudo make uninstall
    cd -
else
    echo  "Berkeley DB is not installed."
fi

echo  "Done!"

echo "[54%] Uninstalling dependency: libprotobuf-dev... "

uninstall_package libprotobuf-dev

echo  "Done!"

echo "[56%] Uninstalling dependency: protobuf-compiler... "

uninstall_package protobuf-compiler

echo  "Done!"

echo "[58%] Uninstalling dependency: libqrencode-dev... "

uninstall_package libqrencode-dev

echo  "Done!"

echo "[60%] Uninstalling dependency: libpng-dev... "

uninstall_package libpng-dev

echo  "Done!"

echo "[62%] Uninstalling QT Components. "

echo "[64%] Uninstalling QT dependency: libqt5gui5... "

uninstall_package libqt5gui5

echo  "Done!"

echo "[66%] Uninstalling QT dependency: libqt5core5a... "

uninstall_package libqt5core5a

echo  "Done!"

echo "[68%] Uninstalling QT dependency: libqt5dbus5... "

uninstall_package libqt5dbus5

echo  "Done!"

echo "[70%] Uninstalling QT dependency: qttools5-dev... "

uninstall_package qttools5-dev

echo  "Done!"

echo "[70%] Uninstalling QT dependency: qttools5-dev-tools... "

uninstall_package qttools5-dev-tools

echo  "Done!"

echo "[72%] Uninstalling QT dependency: libqt5svg5-dev... "

uninstall_package libqt5svg5-dev

echo  "Done!"

echo "[74%] Uninstalling QT dependency: libqt5charts5-dev... "

uninstall_package libqt5charts5-dev

echo  "Done!"

echo "[80%] All QT Components has been uninstalled. "

echo "[90%] Removing of the BTCU sources... "

if [[ -d ./src/qt/btcu ]]
then
    echo "Current filder IS the BTCU project folder. Starting cleaning... "
    mv clean_ubuntu.sh /tmp/ 2> /dev/null
    mv install_ubuntu.sh /tmp/ 2> /dev/null
    rm -rf ./*
    mv /tmp/clean_ubuntu.sh ./ 2> /dev/null
    mv /tmp/install_ubuntu.sh ./ 2> /dev/null
else 
    echo "Current filder IS NOT the BTCU project folder. Stopping...  "
fi

echo  "Done!"
echo  ""
echo  "[100%] The cleaning process is completed!"
echo  "You can reinstall the BTCU project via install_ubuntu.sh script."
