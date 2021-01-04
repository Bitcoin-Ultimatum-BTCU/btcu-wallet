UNIX BUILD NOTES
====================
Some notes on how to build BTCU Core in Unix.

Note
---------------------
Always use absolute paths to configure and compile BTCU Core and the dependencies,
For example, when specifying the path of the dependency:

	../dist/configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX

Here BDB_PREFIX must be an absolute path - it is defined using $(pwd) which ensures
the usage of the absolute path.

To Build
---------------------

```bash
./autogen.sh
./configure
mkdir build
cd build
cmake ..
make
```


This will build btcu-qt as well, if the dependencies are met.

Dependencies
---------------------

These dependencies are required:

 Library     | Purpose            | Description
 ------------|--------------------|----------------------
 libssl      | Crypto             | Random Number Generation, Elliptic Curve Cryptography
 libboost    | Utility            | Library for threading, data structures, etc
 libevent    | Networking         | OS independent asynchronous networking
 libgmp      | Bignum Arithmetic  | Precision arithmetic

Optional dependencies:

 Library     | Purpose          | Description
 ------------|------------------|----------------------
 miniupnpc   | UPnP Support     | Firewall-jumping support
 libdb18.1    | Berkeley DB      | Wallet storage (only needed when wallet enabled)
 qt          | GUI              | GUI toolkit (only needed when GUI enabled)
 protobuf    | Payments in GUI  | Data interchange format used for payment protocol (only needed when GUI enabled)
 univalue    | Utility          | JSON parsing and encoding (bundled version will be used unless --with-system-univalue passed to configure)
 libzmq3     | ZMQ notification | Optional, allows generating ZMQ notifications (requires ZMQ version >= 4.0.0)

For the versions used, see [dependencies.md](dependencies.md)

Memory Requirements
--------------------

C++ compilers are memory-hungry. It is recommended to have at least 1.5 GB of
memory available when compiling BTCU Core. On systems with less, gcc can be
tuned to conserve memory with additional CXXFLAGS:


    ./configure CXXFLAGS="--param ggc-min-expand=1 --param ggc-min-heapsize=32768"


## Linux Distribution Specific Instructions

### Ubuntu & Debian

#### Dependency Build Instructions

Build requirements:

    sudo apt-get install build-essential libtool bsdmainutils autotools-dev autoconf pkg-config automake python3
    
Cmake installing:
    
(minimum required version 3.10)

    sudo apt-get install cmake

or

    sudo apt remove --purge --auto-remove cmake
    wget https://github.com/Kitware/CMake/releases/download/v3.18.4/cmake-3.18.4-Linux-x86_64.tar.gz
    tar -xzvf cmake-3.18.4-Linux-x86_64.tar.gz
    cd cmake-3.18.4-Linux-x86_64/
    ./bootstrap
    make -j$(nproc)
    sudo make install
    
For Ethereum VM smart contracts:

    sudo apt-get install librocksdb-dev libjsoncpp-dev

Now, you can either build from self-compiled [depends](/depends/README.md) or install the required dependencies:

    sudo apt-get install libssl-dev libgmp-dev libevent-dev libboost-all-dev
    
Boost libraries:

Minimum required version 1.65

    sudo apt-get install libboost-all-dev
    
or

    user@user:~$ wget 'http://dl.bintray.com/boostorg/release/1.65.0/source/boost_1_65_0.tar.gz'
    user@user:~$ tar -xvf boost_1_65_0.tar.gz
    user@user:~$ cd <boost-root-directory-after-unpacking>
    user@user:~$ ./bootstrap.sh --prefix=/usr/
    user@user:~$ ./b2
    user@user:~$ sudo ./b2 install

BerkeleyDB is required for the wallet. 
If exists previous version like 4.8 (Bitcoin default) then remove:
    
    sudo apt-get purge libdb4.8-dev libdb4.8++-dev
        
Since as of 5th March 2020 the Oracle moved Barkeley DB to login-protected tarball for 18.1.32 version we added the dependency as a static file included in the repository.

Install:

    tar zxvf depends/packages/static/berkeley-db-18.1.32/berkeley-db-18.1.32.tar.gz -C ./
    cd  db-18.1.32/build_unix
    ../dist/configure --enable-cxx --disable-shared --disable-replication --with-pic --prefix=/opt
    make
    sudo make install

Run btcu project configure:
    
    ./configure BDB_LIBS="-L/opt/lib -ldb_cxx-18.1" BDB_CFLAGS="-I/opt/include"

If you do not care about wallet compatibility, pass `--with-incompatible-bdb` to configure.

Otherwise, you can build from self-compiled `depends` (see above).

To build BTCU Core without wallet, see [*Disable-wallet mode*](/doc/build-unix.md#disable-wallet-mode)


Optional (see --with-miniupnpc and --enable-upnp-default):

    sudo apt-get install libminiupnpc-dev

ZMQ dependencies (provides ZMQ API):

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

    # Expected
    ############################################################
    # libzmq.so.5 (libc6,x86-64) => /usr/local/lib/libzmq.so.5
    # libzmq.so (libc6,x86-64) => /usr/local/lib/libzmq.so
    ############################################################

GUI dependencies:

If you want to build btcu-qt, make sure that the required packages for Qt development
are installed. Qt 5 is necessary to build the GUI.
To build without GUI pass `--without-gui`.

To build with Qt 5 you need the following:

    sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 libqt5svg5-dev libqt5charts5-dev qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler libqrencode-dev libpng-dev

    git clone https://github.com/fukuchi/libqrencode.git
    cd libqrencode
    cmake .
    make
    sudo make install

**Note:** Ubuntu versions prior to Bionic (18.04), and Debian version prior to Buster, do not have the `libqt5charts5-dev` package. If you are compiling on one of these older versions, you will need to omit `libqt5charts5-dev` from the above command.

Once these are installed, they will be found by configure and a btcu-qt executable will be
built by default.


### Fedora

#### Dependency Build Instructions

Build requirements:

    sudo dnf install which gcc-c++ libtool make autoconf automake compat-openssl10-devel libevent-devel boost-devel libdb4-devel libdb4-cxx-devel gmp-devel python3

Optional:

    sudo dnf install miniupnpc-devel zeromq-devel

To build with Qt 5 you need the following:

    sudo dnf install qt5-qttools-devel qt5-qtbase-devel qt5-qtsvg-devel qt5-qtcharts-devel protobuf-devel qrencode-devel libpng-dev

Notes
-----
The release is built with GCC and then "strip btcud" to strip the debug
symbols, which reduces the executable size by about 90%.


miniupnpc
---------

[miniupnpc](http://miniupnp.free.fr/) may be used for UPnP port mapping.  It can be downloaded from [here](
http://miniupnp.tuxfamily.org/files/).  UPnP support is compiled in and
turned off by default.  See the configure options for upnp behavior desired:

	--without-miniupnpc      No UPnP support miniupnp not required
	--disable-upnp-default   (the default) UPnP support turned off by default at runtime
	--enable-upnp-default    UPnP support turned on by default at runtime

To build:

    tar -xzvf miniupnpc-1.6.tar.gz
    cd miniupnpc-1.6
    make
    sudo su
    make install


Berkeley DB
-----------
It is recommended to use Berkeley DB 4.8. If you have to build it yourself,
you can use [the installation script included in contrib/](/contrib/install_db4.sh)
like so:

```shell
./contrib/install_db4.sh `pwd`
```

from the root of the repository.

**Note**: You only need Berkeley DB if the wallet is enabled (see [*Disable-wallet mode*](/doc/build-unix.md#disable-wallet-mode)).

Boost
-----
If you need to build Boost yourself:

	sudo su
	./bootstrap.sh
	./bjam install


Security
--------
To help make your BTCU Core installation more secure by making certain attacks impossible to
exploit even if a vulnerability is found, binaries are hardened by default.
This can be disabled with:

Hardening Flags:

	./configure --enable-hardening
	./configure --disable-hardening


Hardening enables the following features:
* _Position Independent Executable_: Build position independent code to take advantage of Address Space Layout Randomization
    offered by some kernels. Attackers who can cause execution of code at an arbitrary memory
    location are thwarted if they don't know where anything useful is located.
    The stack and heap are randomly located by default, but this allows the code section to be
    randomly located as well.

    On an AMD64 processor where a library was not compiled with -fPIC, this will cause an error
    such as: "relocation R_X86_64_32 against `......' can not be used when making a shared object;"

    To test that you have built PIE executable, install scanelf, part of paxutils, and use:

    	scanelf -e ./btcud

    The output should contain:

     TYPE
    ET_DYN

* _Non-executable Stack_: If the stack is executable then trivial stack-based buffer overflow exploits are possible if
    vulnerable buffers are found. By default, BTCU Core should be built with a non-executable stack
    but if one of the libraries it uses asks for an executable stack or someone makes a mistake
    and uses a compiler extension which requires an executable stack, it will silently build an
    executable without the non-executable stack protection.

    To verify that the stack is non-executable after compiling use:
    `scanelf -e ./btcud`

    The output should contain:
	STK/REL/PTL
	RW- R-- RW-

    The STK RW- means that the stack is readable and writeable but not executable.

Disable-wallet mode
--------------------
**Note:** This functionality is not yet completely implemented, and compilation using the below option will currently fail.

When the intention is to run only a P2P node without a wallet, BTCU Core may be compiled in
disable-wallet mode with:

    ./configure --disable-wallet

In this case there is no dependency on Berkeley DB 18.1.


Additional Configure Flags
--------------------------
A list of additional configure flags can be displayed with:

    ./configure --help


ARM Cross-compilation
-------------------
These steps can be performed on, for example, an Ubuntu VM. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

Make sure you install the build requirements mentioned above.
Then, install the toolchain and curl:

    sudo apt-get install g++-arm-linux-gnueabihf curl

To build executables for ARM:

    cd depends
    make HOST=arm-linux-gnueabihf NO_QT=1
    cd ..
    ./autogen.sh
    ./configure --prefix=$PWD/depends/arm-linux-gnueabihf --enable-glibc-back-compat --enable-reduce-exports LDFLAGS=-static-libstdc++
    make


For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.
