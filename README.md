Bitcoin Ultimatum BTCU Core Main Concepts
=====================================

The concept of BTCU is similar to the concept of the second cryptocurrency by capitalization - Ethereum. 
Like Ethereum, the Bitcoin Ultimatum blockchain provides smart contracts in the blockchain protocol as one of the important functions for the implementation of transactions and DAPP applications. 
The BTCU team sets itself the task of developing the direction of smart contracts and implementing atomic swap technology natively into the blockchain protocol to allow carry out transactions not only within the framework of a single blockchain protocol, 
but also interact with other protocols, which will globally expand the possibilities of this technology. 
The main goal is to make a hybrid of LPoS mining algorithm and PoA to make a real decentralizing infrastructure with limited validators. 


Setup
---------------------
[BTCU Core](https://github.com/Bitcoin-Ultimatum-BTCU/btcu-wallet) is the original BTCU client and it builds the backbone of the network. However, it downloads and stores the entire history of BTCU transactions; depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more. Thankfully you only have to do this once.

Running
---------------------
The following are some helpful notes on how to run BTCU Core on your native platform.

### Unix

Download from [Releases](https://github.com/Bitcoin-Ultimatum-BTCU/btcu-wallet/releases) latest version, unpack the files into a directory and run:

- `btcu-qt` (GUI) or
- `btcud` (headless)

### Windows

Download from [Releases](https://github.com/Bitcoin-Ultimatum-BTCU/btcu-wallet/releases) latest version, unpack the files into a directory and run:
- `btcu-qt.exe` (GUI) or
- `btcud.exe` (headless)

### macOS

Download from [Releases](https://github.com/Bitcoin-Ultimatum-BTCU/btcu-wallet/releases) latest version, unpack the files into a directory and run:
- `BTCU-Qt` (GUI)  

### Need Help?

* See the documentation at the [BTCU Wiki](https://github.com/Bitcoin-Ultimatum-BTCU/btcu-wallet/wiki)
for help and more information.

Building
---------------------
The following are developer notes on how to build BTCU Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](doc/dependencies.md)
- [macOS Build Notes](doc/build-osx.md)
- [Unix Build Notes](doc/build-unix.md)
- [Windows Build Notes](doc/build-windows.md)
- [Gitian Building Guide](doc/gitian-building.md)

Development
---------------------

- [Developer Notes](doc/developer-notes.md)
- [Release Notes](doc/release-notes.md)
- [Release Process](doc/release-process.md)
- [Translation Process](doc/translation_process.md)
- [Unit Tests](doc/unit-tests.md)
- [Unauthenticated REST Interface](doc/REST-interface.md)
- [Dnsseed Policy](doc/dnsseed-policy.md)

### Miscellaneous
- [Assets Attribution](doc/assets-attribution.md)
- [Files](doc/files.md)
- [Tor Support](doc/tor.md)
- [Init Scripts (systemd/upstart/openrc)](doc/init.md)

License
---------------------
Distributed under the [MIT software license](LICENSE).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.

 ### More information at [btcu](http://btcu.app/)
 

