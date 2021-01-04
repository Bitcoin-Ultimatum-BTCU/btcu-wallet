# EVMC

[![chat: on gitter][gitter badge]][Gitter]
[![readme style: standard][readme style standard badge]][standard readme]

> Ethereum Client-VM Connector API

The EVMC is the low-level ABI between Ethereum Virtual Machines (EVMs) and
Ethereum Clients. On the EVM side it supports classic EVM1 and [ewasm].
On the Client-side it defines the interface for EVM implementations
to access Ethereum environment and state.

## Usage

### Documentation

Please visit the [documentation].

### Languages support

| Language                | Supported Versions  |
| ----------------------- | ------------------- |
| **C**                   | C90, C99, C11       |
| **C++** _(helpers)_[^1] | C++11, C++14, C++17 |
| **Go** _(bindings)_     | 1.9, 1.10, 1.11     |

[^1]: C++ support is provided by C headers and some optional C++ helpers.

## Related projects

### EVMs

- [cpp-ethereum-interpreter]
- [evmjit]
- [Hera]

### Clients

- [cpp-ethereum]
- [nim-evmc]
- [go-ethereum] (in progress)
- [pyevm] (in progress)
- [pyethereum] (abandoned)


## Maintainers

- Alex Beregszaszi [@axic]
- Paweł Bylica [@chfast]

See also the list of [EVMC Authors](AUTHORS.md).

## Contributing

[![chat: on gitter][gitter badge]][Gitter]

Talk with us on the [EVMC Gitter chat][Gitter].

## License

Licensed under the [MIT License](LICENSE).


## Internal

### Making new release

1. Update [CHANGELOG.md](CHANGELOG.md), put the release date, update release link.
2. `git add CHANGELOG.md`.
3. Tag new release: `bumpversion --allow-dirty prerel`.
4. Prepare CHANGELOG for next release: add unreleased section and link.
5. `git add CHANGELOG.md`.
6. Start new release series: `bumpversion --allow-dirty --no-tag minor`.


[@axic]: https://github.com/axic
[@chfast]: https://github.com/chfast
[documentation]: https://ethereum.github.io/evmc
[ewasm]: https://github.com/ewasm/design
[evmjit]: https://github.com/ethereum/evmjit
[Hera]: https://github.com/ewasm/hera
[Gitter]: https://gitter.im/ethereum/evmc
[cpp-ethereum-interpreter]: https://github.com/ethereum/cpp-ethereum/tree/master/libaleth-interpreter
[cpp-ethereum]: https://github.com/ethereum/cpp-ethereum
[nim-evmc]: https://github.com/status-im/nim-evmc
[go-ethereum]: https://github.com/ethereum/go-ethereum/pull/17050
[pyevm]: https://github.com/ethereum/py-evm
[pyethereum]: https://github.com/ethereum/pyethereum/pull/406
[standard readme]: https://github.com/RichardLitt/standard-readme

[gitter badge]: https://img.shields.io/gitter/room/ethereum/evmc.svg?style=flat-square
[readme style standard badge]: https://img.shields.io/badge/readme%20style-standard-brightgreen.svg?style=flat-square
