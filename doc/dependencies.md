Dependencies
============

These are the dependencies currently used by BTCU. You can find instructions for installing them in the [`build-*.md`](../INSTALL.md) file for your platform.

| Dependency | Version used | Minimum required | CVEs | Shared | [Bundled Qt library](https://doc.qt.io/qt-5/configure-options.html) |
| --- | --- | --- | --- | --- | --- |
| Berkeley DB | [18.1.32](https://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index.html) |  | No |  |  |
| Boost | [1.71.0](https://www.boost.org/users/download/) | 1.65.0 | No |  |  |
| Clang |  | [5](https://releases.llvm.org/download.html) (C++17 support) |  |  |  |
| D-Bus | [1.10.18](https://cgit.freedesktop.org/dbus/dbus/tree/NEWS?h=dbus-1.10) |  | No | Yes |  |
| CMake |  | [3.16](https://cmake.org/download/) |  |  |  |
| Expat | [2.2.7](https://libexpat.github.io/) |  | No | Yes |  |
| fontconfig | [2.12.6](https://www.freedesktop.org/software/fontconfig/release/) |  | No | Yes |  |
| FreeType | [2.7.1](https://download.savannah.gnu.org/releases/freetype) |  | No |  |  |
| GCC |  | [7](https://gcc.gnu.org/) (C++11 support) |  |  |  |
| HarfBuzz-NG |  |  |  |  |  |
| libevent | [2.1.11-stable](https://github.com/libevent/libevent/releases) | 2.0.22 | No |  |  |
| libjpeg |  |  |  |  | [Yes](https://github.com/bitcoin-ultimatum/orion/blob/master/depends/packages/qt.mk#L74) |
| libpng |  |  |  |  | [Yes](https://github.com/bitcoin-ultimatum/orion/blob/master/depends/packages/qt.mk#L73) |
| librsvg | |  |  |  |  |
| MiniUPnPc | [2.0.20180203](https://miniupnp.tuxfamily.org/files) | 1.9 | No |  |  |
| OpenSSL | [1.0.1k](https://www.openssl.org/source) |  | Yes |  |  |
| GMP | [6.1.2](https://gmplib.org/) | | No | | |
| PCRE |  |  |  |  | [Yes](https://github.com/bitcoin-ultimatum/orion/blob/master/depends/packages/qt.mk#L75) |
| protobuf | [2.6.1](https://github.com/google/protobuf/releases) |  | No |  |  |
| Python |  | [3.5](https://www.python.org/downloads) |  |  |  |
| qrencode | [3.4.4](https://fukuchi.org/works/qrencode) |  | No |  |  |
| Qt | [5.9.7](https://download.qt.io/official_releases/qt/) | 5.5.1 | No |  |  |
| XCB |  |  |  |  | [Yes](https://github.com/bitcoin-ultimatum/orion/blob/master/depends/packages/qt.mk#L108) (Linux only) |
| xkbcommon |  |  |  |  | [Yes](https://github.com/bitcoin-ultimatum/orion/blob/master/depends/packages/qt.mk#L107) (Linux only) |
| ZeroMQ | [4.3.1](https://github.com/zeromq/libzmq/releases) | 4.0.0 | No |  |  |
| zlib | [1.2.11](https://zlib.net/) |  |  |  | No |

Controlling dependencies
------------------------
Some dependencies are not needed in all configurations. The following are some factors that affect the dependency list.

#### Options passed to `cmake`
* MiniUPnPc is not needed with  `-DWITH_MINIUNPC=OFF`.
* Berkeley DB is not needed with `-DENABLE_WALLET=OFF`.
* Qt is not needed with `-DENABLE_GUI=OFF`.
* ZeroMQ is not needed with the `-DENABLE_ZMQ=OFF`.

#### Other
* librsvg is only needed if you need to run `make osx-dmg` on
  (cross-compilation to) macOS.
