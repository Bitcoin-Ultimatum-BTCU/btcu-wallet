# Copyright (c) 2013-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

DIST_SUBDIRS = secp256k1 univalue

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(HARDENED_LDFLAGS) $(GPROF_LDFLAGS) $(SANITIZER_LDFLAGS)
AM_CXXFLAGS = $(DEBUG_CXXFLAGS) $(HARDENED_CXXFLAGS) $(WARN_CXXFLAGS) $(NOWARN_CXXFLAGS) $(ERROR_CXXFLAGS) $(GPROF_CXXFLAGS) $(SANITIZER_CXXFLAGS)
AM_CPPFLAGS = $(DEBUG_CPPFLAGS) $(HARDENED_CPPFLAGS)
AM_LIBTOOLFLAGS = --preserve-dup-deps
EXTRA_LIBRARIES =

if EMBEDDED_UNIVALUE
LIBUNIVALUE = univalue/libunivalue.la

$(LIBUNIVALUE): $(wildcard univalue/lib/*) $(wildcard univalue/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)
else
LIBUNIVALUE = $(UNIVALUE_LIBS)
endif

BITCOIN_INCLUDES=-I$(builddir) $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(LEVELDB_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS) -I$(srcdir)/cpp-ethereum/utils

BITCOIN_INCLUDES += -I$(srcdir)/secp256k1/include
BITCOIN_INCLUDES += -I$(srcdir)/libff/libff
BITCOIN_INCLUDES += -I$(srcdir)/libff
BITCOIN_INCLUDES += $(UNIVALUE_CFLAGS)
BITCOIN_INCLUDES += -I/usr/include/jsoncpp

BITCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum
BITCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum/evmc/include
BITCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum/ethash/include
BITCOIN_INCLUDES += -I$(srcdir)/cpp-ethereum/utils/libscrypt

LIBBITCOIN_SERVER=libbitcoin_server.a
LIBBITCOIN_COMMON=libbitcoin_common.a
LIBBITCOIN_CLI=libbitcoin_cli.a
LIBBITCOIN_UTIL=libbitcoin_util.a
LIBBITCOIN_CRYPTO=crypto/libbitcoin_crypto.a
LIBBITCOIN_ZEROCOIN=libzerocoin/libbitcoin_zerocoin.a
LIBBITCOINQT=qt/libbitcoinqt.a
LIBSECP256K1=secp256k1/libsecp256k1.la
LIBCRYPTOPP=cryptopp/libcryptopp.a
LIBFF=libff/libff.a

if ENABLE_ZMQ
LIBBITCOIN_ZMQ=libbitcoin_zmq.a
endif
if BUILD_BITCOIN_LIBS
LIBBITCOINCONSENSUS=libbitcoinconsensus.la
endif
if ENABLE_WALLET
LIBBITCOIN_WALLET=libbitcoin_wallet.a
endif

$(LIBSECP256K1): $(wildcard secp256k1/src/*.h) $(wildcard secp256k1/src/*.c) $(wildcard secp256k1/include/*)
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(@D) $(@F)

# Make is not made aware of per-object dependencies to avoid limiting building parallelization
# But to build the less dependent modules first, we manually select their order here:
EXTRA_LIBRARIES += \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_ZEROCOIN) \
  $(LIBBITCOIN_SERVER) \
  $(LIBBITCOIN_CLI) \
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_ZMQ)

lib_LTLIBRARIES = $(LIBBITCOINCONSENSUS)

bin_PROGRAMS =
noinst_PROGRAMS =
TESTS =
BENCHMARKS =

if BUILD_BITCOIND
  bin_PROGRAMS += btcud
endif

if BUILD_BITCOIN_UTILS
  bin_PROGRAMS += btcu-cli btcu-tx
endif

.PHONY: FORCE check-symbols check-security
# btcu core #
BITCOIN_CORE_H = \
  activemasternode.h \
  addrman.h \
  alert.h \
  allocators.h \
  amount.h \
  base58.h \
  bech32.h \
  btcu_address.h \
  bip38.h \
  bloom.h \
  blocksignature.h \
  chain.h \
  chainparams.h \
  chainparamsbase.h \
  chainparamsseeds.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  compat.h \
  compat/byteswap.h \
  compat/endian.h \
  compat/sanity.h \
  compressor.h \
  consensus/consensus.h \
  consensus/merkle.h \
  consensus/validation.h \
  consensus/tx_verify.h \
  primitives/block.h \
  primitives/transaction.h \
  core_io.h \
  crypter.h \
  pairresult.h \
  addressbook.h \
  denomination_functions.h \
  obfuscation.h \
  obfuscation-relay.h \
  wallet/db.h \
  hash.h \
  httprpc.h \
  httpserver.h \
  init.h \
  invalid.h \
  invalid_outpoints.json.h \
  invalid_serials.json.h \
  kernel.h \
  swifttx.h \
  key.h \
  keystore.h \
  leveldbwrapper.h \
  limitedmap.h \
  main.h \
  masternode.h \
  masternode-payments.h \
  masternode-budget.h \
  masternode-sync.h \
  masternodeman.h \
  masternodeconfig.h \
  merkleblock.h \
  messagesigner.h \
  miner.h \
  mruset.h \
  netbase.h \
  net.h \
  noui.h \
  pow.h \
  protocol.h \
  pubkey.h \
  random.h \
  reverselock.h \
  reverse_iterate.h \
  rpc/client.h \
  rpc/protocol.h \
  rpc/server.h \
  scheduler.h \
  script/interpreter.h \
  script/script.h \
  script/sigcache.h \
  script/sign.h \
  script/standard.h \
  script/script_error.h \
  serialize.h \
  spork.h \
  sporkdb.h \
  sporkid.h \
  stakeinput.h \
  streams.h \
  support/cleanse.h \
  sync.h \
  threadsafety.h \
  timedata.h \
  tinyformat.h \
  torcontrol.h \
  txdb.h \
  txmempool.h \
  guiinterface.h \
  uint256.h \
  undo.h \
  util/convert.h \
  util.h \
  utilstrencodings.h \
  utilmoneystr.h \
  utiltime.h \
  validationinterface.h \
  version.h \
  wallet/wallet.h \
  wallet/wallet_ismine.h \
  wallet/walletdb.h \
  leasing/leasingmanager.h \
  leasing/leasing_tx_verify.h \
  zbtcuchain.h \
  zbtcu/deterministicmint.h \
  zbtcu/mintpool.h \
  zbtcu/zerocoin.h \
  zbtcu/zbtcutracker.h \
  zbtcu/zbtcuwallet.h \
  zbtcu/zbtcumodule.h \
  zbtcu/zpos.h \
  zmq/zmqabstractnotifier.h \
  zmq/zmqconfig.h \
  zmq/zmqnotificationinterface.h \
  zmq/zmqpublishnotifier.h \
  qtum/qtumDGP.h

obj/build.h: FORCE
	@$(MKDIR_P) $(builddir)/obj
	@$(top_srcdir)/share/genbuild.sh "$(abs_top_builddir)/src/obj/build.h" \
	  "$(abs_top_srcdir)"
libbitcoin_util_a-clientversion.$(OBJEXT): obj/build.h

# server: shared between btcud and btcu-qt
libbitcoin_server_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(EVENT_PTHREADS_CFLAGS)
libbitcoin_server_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_server_a_SOURCES = \
  addrman.cpp \
  alert.cpp \
  bloom.cpp \
  blocksignature.cpp \
  chain.cpp \
  checkpoints.cpp \
  consensus/tx_verify.cpp \
  consensus/validator_tx_verify.cpp \
  httprpc.cpp \
  httpserver.cpp \
  init.cpp \
  leveldbwrapper.cpp \
  main.cpp \
  merkleblock.cpp \
  miner.cpp \
  net.cpp \
  noui.cpp \
  pow.cpp \
  rest.cpp \
  rpc/blockchain.cpp \
  rpc/masternode.cpp \
  rpc/budget.cpp \
  rpc/mining.cpp \
  rpc/misc.cpp \
  rpc/net.cpp \
  rpc/rawtransaction.cpp \
  rpc/server.cpp \
  rpc/validators.cpp \
  masternode-validators.cpp \
  script/sigcache.cpp \
  sporkdb.cpp \
  timedata.cpp \
  torcontrol.cpp \
  txdb.cpp \
  txmempool.cpp \
  validationinterface.cpp \
  zbtcuchain.cpp \
  qtum/qtumDGP.cpp \
  $(BITCOIN_CORE_H)

if ENABLE_ZMQ
libbitcoin_zmq_a_CPPFLAGS = $(BITCOIN_INCLUDES) $(ZMQ_CFLAGS)
libbitcoin_zmq_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_zmq_a_SOURCES = \
  zmq/zmqabstractnotifier.cpp \
  zmq/zmqnotificationinterface.cpp \
  zmq/zmqpublishnotifier.cpp
endif

# wallet: shared between btcud and btcu-qt, but only linked
# when wallet enabled
libbitcoin_wallet_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_wallet_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_wallet_a_SOURCES = \
  activemasternode.cpp \
  bip38.cpp \
  denomination_functions.cpp \
  obfuscation.cpp \
  obfuscation-relay.cpp \
  wallet/db.cpp \
  addressbook.cpp \
  crypter.cpp \
  swifttx.cpp \
  masternode.cpp \
  masternode-budget.cpp \
  masternode-payments.cpp \
  masternode-sync.cpp \
  masternodeconfig.cpp \
  masternodeman.cpp \
  messagesigner.cpp \
  wallet/rpcdump.cpp \
  wallet/rpcwallet.cpp \
  kernel.cpp \
  wallet/wallet.cpp \
  wallet/wallet_ismine.cpp \
  wallet/wallet_zerocoin.cpp \
  wallet/walletdb.cpp \
  leasing/leasingmanager.cpp \
  leasing/leasing_tx_verify.cpp \
  zbtcu/deterministicmint.cpp \
  zbtcu/zerocoin.cpp \
  zbtcu/mintpool.cpp \
  zbtcu/zbtcuwallet.cpp \
  zbtcu/zbtcutracker.cpp \
  stakeinput.cpp \
  zbtcu/zbtcumodule.cpp \
  zbtcu/zpos.cpp \
  $(BITCOIN_CORE_H)

# crypto primitives library
crypto_libbitcoin_crypto_a_CPPFLAGS = $(AM_CPPFLAGS) $(PIC_FLAGS)
crypto_libbitcoin_crypto_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIC_FLAGS)
crypto_libbitcoin_crypto_a_SOURCES = \
  crypto/sha1.cpp \
  crypto/sha256.cpp \
  crypto/sha512.cpp \
  crypto/chacha20.h \
  crypto/chacha20.cpp \
  crypto/hmac_sha256.cpp \
  crypto/rfc6979_hmac_sha256.cpp \
  crypto/hmac_sha512.cpp \
  crypto/scrypt.cpp \
  crypto/ripemd160.cpp \
  crypto/aes_helper.c \
  crypto/blake.c \
  crypto/bmw.c \
  crypto/groestl.c \
  crypto/jh.c \
  crypto/keccak.c \
  crypto/skein.c \
  crypto/common.h \
  crypto/sha256.h \
  crypto/sha512.h \
  crypto/hmac_sha256.h \
  crypto/rfc6979_hmac_sha256.h \
  crypto/hmac_sha512.h \
  crypto/scrypt.h \
  crypto/sha1.h \
  crypto/ripemd160.h \
  crypto/sph_blake.h \
  crypto/sph_bmw.h \
  crypto/sph_groestl.h \
  crypto/sph_jh.h \
  crypto/sph_keccak.h \
  crypto/sph_skein.h \
  crypto/sph_types.h \
  cpp-ethereum/utils/libscrypt/b64.c \
  cpp-ethereum/utils/libscrypt/b64.h \
  cpp-ethereum/utils/libscrypt/crypto-mcf.c \
  cpp-ethereum/utils/libscrypt/crypto_scrypt-check.c \
  cpp-ethereum/utils/libscrypt/crypto_scrypt-hash.c \
  cpp-ethereum/utils/libscrypt/crypto_scrypt-hexconvert.c \
  cpp-ethereum/utils/libscrypt/crypto_scrypt-hexconvert.h \
  cpp-ethereum/utils/libscrypt/crypto_scrypt-nosse.c \
  cpp-ethereum/utils/libscrypt/crypto-scrypt-saltgen.c \
  cpp-ethereum/utils/libscrypt/libscrypt.h \
  cpp-ethereum/utils/libscrypt/sha256.c \
  cpp-ethereum/utils/libscrypt/sha256.h \
  cpp-ethereum/utils/libscrypt/slowequals.c \
  cpp-ethereum/utils/libscrypt/slowequals.h \
  cpp-ethereum/utils/libscrypt/sysendian.h

# libzerocoin library
libzerocoin_libbitcoin_zerocoin_a_CPPFLAGS = $(AM_CPPFLAGS) $(BOOST_CPPFLAGS)
libzerocoin_libbitcoin_zerocoin_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libzerocoin_libbitcoin_zerocoin_a_SOURCES = \
  libzerocoin/Accumulator.h \
  libzerocoin/bignum.h \
  libzerocoin/Coin.h \
  libzerocoin/CoinRandomnessSchnorrSignature.h \
  libzerocoin/CoinSpend.h \
  libzerocoin/Commitment.h \
  libzerocoin/Denominations.h \
  libzerocoin/ParamGeneration.h \
  libzerocoin/Params.h \
  libzerocoin/SpendType.h \
  libzerocoin/ZerocoinDefines.h \
  libzerocoin/bignum.cpp \
  libzerocoin/Accumulator.cpp \
  libzerocoin/Coin.cpp \
  libzerocoin/CoinRandomnessSchnorrSignature.cpp \
  libzerocoin/CoinSpend.cpp \
  libzerocoin/Denominations.cpp \
  libzerocoin/ParamGeneration.cpp \
  libzerocoin/Params.cpp
if USE_NUM_GMP
  libzerocoin_libbitcoin_zerocoin_a_SOURCES += libzerocoin/bignum_gmp.cpp
endif
if USE_NUM_OPENSSL
  libzerocoin_libbitcoin_zerocoin_a_SOURCES += libzerocoin/bignum_openssl.cpp
endif

# common: shared between btcud, and btcu-qt and non-server tools
libbitcoin_common_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_common_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_common_a_SOURCES = \
  allocators.cpp \
  amount.cpp \
  base58.cpp \
  bech32.cpp \
  btcu_address.cpp \
  bip38.cpp \
  chainparams.cpp \
  coins.cpp \
  compressor.cpp \
  consensus/merkle.cpp \
  consensus/tx_verify.cpp \
  primitives/block.cpp \
  zbtcu/deterministicmint.cpp \
  primitives/transaction.cpp \
  zbtcu/zerocoin.cpp \
  core_read.cpp \
  core_write.cpp \
  hash.cpp \
  invalid.cpp \
  key.cpp \
  keystore.cpp \
  netbase.cpp \
  protocol.cpp \
  pubkey.cpp \
  scheduler.cpp \
  script/interpreter.cpp \
  script/script.cpp \
  script/sign.cpp \
  script/standard.cpp \
  script/script_error.cpp \
  spork.cpp \
  sporkdb.cpp \
  validators_state.cpp \
  validators_voting.cpp \
  cpp-ethereum/aleth/buildinfo.c \
  cpp-ethereum/aleth/buildinfo.h \
  cpp-ethereum/evmc/lib/instructions/instruction_metrics.c \
  cpp-ethereum/evmc/lib/instructions/instruction_names.c \
  cpp-ethereum/libdevcore/Address.cpp \
  cpp-ethereum/libdevcore/Address.h \
  cpp-ethereum/libdevcore/Base64.cpp \
  cpp-ethereum/libdevcore/Base64.h \
  cpp-ethereum/libdevcore/Common.cpp \
  cpp-ethereum/libdevcore/Common.h \
  cpp-ethereum/libdevcore/CommonData.cpp \
  cpp-ethereum/libdevcore/CommonData.h \
  cpp-ethereum/libdevcore/CommonIO.cpp \
  cpp-ethereum/libdevcore/CommonIO.h \
  cpp-ethereum/libdevcore/CommonJS.cpp \
  cpp-ethereum/libdevcore/CommonJS.h \
  cpp-ethereum/libdevcore/FileSystem.cpp \
  cpp-ethereum/libdevcore/FileSystem.h \
  cpp-ethereum/libdevcore/FixedHash.cpp \
  cpp-ethereum/libdevcore/FixedHash.h \
  cpp-ethereum/libdevcore/Guards.cpp \
  cpp-ethereum/libdevcore/Guards.h \
  cpp-ethereum/libdevcrypto/Hash.cpp \
  cpp-ethereum/libdevcrypto/Hash.h \
  cpp-ethereum/libdevcore/Log.cpp \
  cpp-ethereum/libdevcore/Log.h \
  cpp-ethereum/libdevcore/LevelDB.cpp \
  cpp-ethereum/libdevcore/LevelDB.h \
  cpp-ethereum/libdevcore/MemoryDB.cpp \
  cpp-ethereum/libdevcore/MemoryDB.h \
  cpp-ethereum/libdevcore/OverlayDB.cpp \
  cpp-ethereum/libdevcore/OverlayDB.h \
  cpp-ethereum/libdevcore/StateCacheDB.cpp \
  cpp-ethereum/libdevcore/StateCacheDB.h \
  cpp-ethereum/libdevcore/RLP.cpp \
  cpp-ethereum/libdevcore/RLP.h \
  cpp-ethereum/libdevcore/SHA3.cpp \
  cpp-ethereum/libdevcore/SHA3.h \
  cpp-ethereum/libdevcore/TransientDirectory.cpp \
  cpp-ethereum/libdevcore/TransientDirectory.h \
  cpp-ethereum/libdevcore/TrieCommon.cpp \
  cpp-ethereum/libdevcore/TrieCommon.h \
  cpp-ethereum/libdevcore/Worker.cpp \
  cpp-ethereum/libdevcore/Worker.h \
  cpp-ethereum/libdevcore/DBFactory.h \
  cpp-ethereum/libdevcore/DBFactory.cpp \
  cpp-ethereum/libdevcore/JsonUtils.h \
  cpp-ethereum/libdevcore/JsonUtils.cpp \
  cpp-ethereum/libdevcore/dbfwd.h \
  cpp-ethereum/libevm/EVMC.cpp \
  cpp-ethereum/libevm/EVMC.h \
  cpp-ethereum/libevm/ExtVMFace.cpp \
  cpp-ethereum/libevm/ExtVMFace.h \
  cpp-ethereum/libaleth-interpreter/interpreter.h \
  cpp-ethereum/libaleth-interpreter/VMConfig.h \
  cpp-ethereum/libaleth-interpreter/VM.cpp \
  cpp-ethereum/libaleth-interpreter/VM.h \
  cpp-ethereum/libaleth-interpreter/VMOpt.cpp \
  cpp-ethereum/libaleth-interpreter/VMCalls.cpp \
  cpp-ethereum/libevm/VMFactory.cpp \
  cpp-ethereum/libevm/VMFactory.h \
  cpp-ethereum/libevm/Instruction.cpp \
  cpp-ethereum/libevm/Instruction.h \
  cpp-ethereum/libevm/LegacyVM.h \
  cpp-ethereum/libevm/LegacyVM.cpp \
  cpp-ethereum/libevm/LegacyVMCalls.cpp \
  cpp-ethereum/libevm/LegacyVMConfig.h \
  cpp-ethereum/libevm/LegacyVMOpt.cpp \
  cpp-ethereum/libethcore/EVMSchedule.h \
  cpp-ethereum/libethereum/ImportPerformanceLogger.cpp \
  cpp-ethereum/libethereum/ImportPerformanceLogger.h \
  cpp-ethereum/libethereum/Account.cpp \
  cpp-ethereum/libethereum/GasPricer.cpp \
  cpp-ethereum/libethereum/State.cpp \
  cpp-ethereum/libethcore/ABI.cpp \
  cpp-ethereum/libethcore/ChainOperationParams.cpp \
  cpp-ethereum/libethcore/Common.cpp \
  cpp-ethereum/libethcore/Precompiled.cpp \
  cpp-ethereum/libethcore/TransactionBase.cpp \
  cpp-ethereum/libdevcrypto/Common.cpp \
  cpp-ethereum/libdevcrypto/CryptoPP.cpp \
  cpp-ethereum/libdevcrypto/AES.cpp \
  cpp-ethereum/libdevcrypto/LibSnark.cpp \
  cpp-ethereum/libethashseal/GenesisInfo.cpp \
  cpp-ethereum/libethereum/ChainParams.cpp \
  cpp-ethereum/libethereum/Transaction.cpp \
  cpp-ethereum/libethereum/Executive.cpp \
  cpp-ethereum/libethereum/ExtVM.cpp \
  cpp-ethereum/libethereum/Block.cpp \
  cpp-ethereum/libethereum/BlockChain.cpp \
  cpp-ethereum/libethereum/BlockDetails.cpp \
  cpp-ethereum/libethereum/TransactionQueue.cpp \
  cpp-ethereum/libethereum/BlockQueue.cpp \
  cpp-ethereum/libethcore/BlockHeader.cpp \
  cpp-ethereum/libdevcore/RLP.cpp \
  cpp-ethereum/libethereum/TransactionReceipt.cpp \
  cpp-ethereum/libethcore/SealEngine.cpp \
  cpp-ethereum/libdevcore/TrieHash.cpp \
  cpp-ethereum/libethereum/GenesisInfo.cpp \
  cpp-ethereum/libethashseal/Ethash.cpp \
  cpp-ethereum/libethashseal/EthashCPUMiner.cpp \
  cpp-ethereum/libethashseal/EthashProofOfWork.cpp \
  cpp-ethereum/libethereum/ValidationSchemes.h \
  cpp-ethereum/libethereum/ValidationSchemes.cpp \
  cpp-ethereum/libethereum/Account.h \
  cpp-ethereum/libethereum/GasPricer.h \
  cpp-ethereum/libethereum/SecureTrieDB.h \
  cpp-ethereum/libethereum/State.h \
  cpp-ethereum/libethcore/ABI.h \
  cpp-ethereum/libethcore/ChainOperationParams.h \
  cpp-ethereum/libethcore/Common.h \
  cpp-ethereum/libethcore/Precompiled.h \
  cpp-ethereum/libethcore/LogEntry.h \
  cpp-ethereum/libethcore/LogEntry.cpp \
  cpp-ethereum/libdevcore/TrieDB.h \
  cpp-ethereum/libdevcrypto/Common.h \
  cpp-ethereum/libdevcrypto/CryptoPP.h \
  cpp-ethereum/libdevcrypto/AES.h \
  cpp-ethereum/libdevcrypto/LibSnark.h \
  cpp-ethereum/libethashseal/GenesisInfo.h \
  cpp-ethereum/libethereum/ChainParams.h \
  cpp-ethereum/libethcore/Exceptions.h \
  cpp-ethereum/libethcore/TransactionBase.h \
  cpp-ethereum/libethereum/Transaction.h \
  cpp-ethereum/libethereum/Executive.h \
  cpp-ethereum/libethereum/ExtVM.h \
  cpp-ethereum/libethereum/Block.h \
  cpp-ethereum/libethereum/BlockChain.h \
  cpp-ethereum/libethereum/BlockDetails.h \
  cpp-ethereum/libethereum/TransactionQueue.h \
  cpp-ethereum/libethereum/BlockQueue.h \
  cpp-ethereum/libethcore/BlockHeader.h \
  cpp-ethereum/libdevcore/RLP.h \
  cpp-ethereum/libethereum/TransactionReceipt.h \
  cpp-ethereum/libethcore/SealEngine.h \
  cpp-ethereum/libdevcore/TrieHash.h \
  cpp-ethereum/libethereum/GenesisInfo.h \
  cpp-ethereum/libethashseal/Ethash.h \
  cpp-ethereum/libethashseal/EthashCPUMiner.h \
  cpp-ethereum/libethashseal/EthashProofOfWork.h \
  cpp-ethereum/libdevcore/vector_ref.h \
  cpp-ethereum/libdevcore/Exceptions.h \
  cpp-ethereum/libdevcore/db.h \
  cpp-ethereum/libdevcore/concurrent_queue.h \
  cpp-ethereum/libdevcore/Terminal.h \
  cpp-ethereum/libethereum/CodeSizeCache.h \
  cpp-ethereum/libethereum/VerifiedBlock.h \
  cpp-ethereum/libdevcore/Assertions.h \
  cpp-ethereum/libethereum/GenericMiner.h \
  cpp-ethereum/libevm/VMFace.h \
  cpp-ethereum/libethereum/GenericFarm.h \
  cpp-ethereum/libethereum/LastBlockHashesFace.h \
  cpp-ethereum/libethereum/Interface.h \
  cpp-ethereum/libethereum/Client.h \
  cpp-ethereum/libethcore/CommonJS.h \
  cpp-ethereum/libdevcrypto/Exceptions.h \
  cpp-ethereum/libethashseal/genesis/mainNetwork.cpp \
  cpp-ethereum/libethashseal/genesis/qtumMainNetwork.cpp \
  cpp-ethereum/libethashseal/genesis/ropsten.cpp \
  cpp-ethereum/libethashseal/genesis/test/qtumTestNetwork.cpp \
  cpp-ethereum/libethashseal/genesis/test/transitionnetTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/mainNetworkTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/mainNetworkNoProofTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/homesteadToEIP150At5Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/homesteadToDaoAt5Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/homesteadTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/frontierToHomesteadAt5Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/frontierTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/frontierNoProofTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/experimentalTransitionTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/EIP158ToByzantiumAt5Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/eip158Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/eip150Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/constantinopleTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/constantinopleNoProofTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/byzantiumTransitionTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/ByzantiumToConstantinopleFixAt5Test.cpp \
  cpp-ethereum/libethashseal/genesis/test/byzantiumTest.cpp \
  cpp-ethereum/libethashseal/genesis/test/byzantiumNoProofTest.cpp \
  cpp-ethereum/utils/json_spirit/JsonSpiritHeaders.h \
  cpp-ethereum/utils/json_spirit/json_spirit.h \
  cpp-ethereum/utils/json_spirit/json_spirit_error_position.h \
  cpp-ethereum/utils/json_spirit/json_spirit_reader.h \
  cpp-ethereum/utils/json_spirit/json_spirit_reader_template.h \
  cpp-ethereum/utils/json_spirit/json_spirit_stream_reader.h \
  cpp-ethereum/utils/json_spirit/json_spirit_utils.h \
  cpp-ethereum/utils/json_spirit/json_spirit_value.h \
  cpp-ethereum/utils/json_spirit/json_spirit_writer.h \
  cpp-ethereum/utils/json_spirit/json_spirit_writer_template.h \
  cpp-ethereum/evmc/include/evmc/evmc.h \
  cpp-ethereum/evmc/include/evmc/evmc.hpp \
  cpp-ethereum/evmc/include/evmc/helpers.h \
  cpp-ethereum/evmc/include/evmc/helpers.hpp \
  cpp-ethereum/evmc/include/evmc/instructions.h \
  cpp-ethereum/evmc/include/evmc/loader.h \
  cpp-ethereum/evmc/include/evmc/utils.h \
  cpp-ethereum/ethash/include/ethash/ethash.h \
  cpp-ethereum/ethash/include/ethash/ethash.hpp \
  cpp-ethereum/ethash/include/ethash/hash_types.h \
  cpp-ethereum/ethash/include/ethash/hash_types.hpp \
  cpp-ethereum/ethash/include/ethash/keccak.h \
  cpp-ethereum/ethash/include/ethash/keccak.hpp \
  cpp-ethereum/ethash/include/ethash/progpow.hpp \
  cpp-ethereum/ethash/lib/ethash/support/attributes.h \
  cpp-ethereum/ethash/lib/ethash/bit_manipulation.h \
  cpp-ethereum/ethash/lib/ethash/builtins.h \
  cpp-ethereum/ethash/lib/ethash/endianness.hpp \
  cpp-ethereum/ethash/lib/ethash/ethash.cpp \
  cpp-ethereum/ethash/lib/ethash/ethash-internal.hpp \
  cpp-ethereum/ethash/lib/ethash/keccak.c \
  cpp-ethereum/ethash/lib/ethash/keccakf800.c \
  cpp-ethereum/ethash/lib/ethash/keccakf1600.c \
  cpp-ethereum/ethash/lib/ethash/kiss99.hpp \
  cpp-ethereum/ethash/lib/ethash/managed.cpp \
  cpp-ethereum/ethash/lib/ethash/primes.c \
  cpp-ethereum/ethash/lib/ethash/primes.h \
  cpp-ethereum/ethash/lib/ethash/progpow.cpp \
  libff/libff/algebra/curves/public_params.hpp \
  libff/libff/algebra/curves/curve_utils.hpp \
  cpp-ethereum/libethereum/BlockChainImporter.h \
  cpp-ethereum/libethereum/EthereumCapability.h \
  cpp-ethereum/libethereum/ImportPerformanceLogger.h \
  cpp-ethereum/libethereum/SnapshotImporter.h \
  cpp-ethereum/libethereum/SnapshotStorage.h \
  cpp-ethereum/libethereum/StateImporter.h \
  cpp-ethereum/libethereum/ValidationSchemes.h \
  cpp-ethereum/libethereum/WarpCapability.h \
  cpp-ethereum/libethereum/ClientBase.h \
  cpp-ethereum/libethereum/ClientTest.h \
  cpp-ethereum/libethereum/LogFilter.h \
  cpp-ethereum/libethereum/CommonNet.h \
  cpp-ethereum/libp2p/All.h \
  cpp-ethereum/libp2p/Capability.h \
  cpp-ethereum/libp2p/CapabilityHost.h \
  cpp-ethereum/libp2p/Common.h \
  cpp-ethereum/libp2p/Host.h \
  cpp-ethereum/libp2p/Network.h \
  cpp-ethereum/libp2p/NodeTable.h \
  cpp-ethereum/libp2p/Peer.h \
  cpp-ethereum/libp2p/RLPXFrameCoder.h \
  cpp-ethereum/libp2p/RLPXPacket.h \
  cpp-ethereum/libp2p/RLPXSocket.h \
  cpp-ethereum/libp2p/RLPxHandshake.h \
  cpp-ethereum/libp2p/Session.h \
  cpp-ethereum/libp2p/UDP.h \
  cpp-ethereum/libp2p/UPnP.h \
  cpp-ethereum/libethashseal/genesis/test/constantinopleFixTest.cpp \
  $(BITCOIN_CORE_H)

# util: shared between all executables.
# This library *must* be included to make sure that the glibc
# backward-compatibility objects and their sanity checks are linked.
libbitcoin_util_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_util_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_util_a_SOURCES = \
  allocators.cpp \
  chainparamsbase.cpp \
  clientversion.cpp \
  compat/glibc_sanity.cpp \
  compat/glibcxx_sanity.cpp \
  compat/strnlen.cpp \
  random.cpp \
  rpc/protocol.cpp \
  support/cleanse.cpp \
  sync.cpp \
  uint256.cpp \
  util.cpp \
  utilmoneystr.cpp \
  utilstrencodings.cpp \
  utiltime.cpp \
  $(BITCOIN_CORE_H)

if GLIBC_BACK_COMPAT
libbitcoin_util_a_SOURCES += compat/glibc_compat.cpp
AM_LDFLAGS += $(COMPAT_LDFLAGS)
endif

# cli: shared between btcu-cli and btcu-qt
libbitcoin_cli_a_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
libbitcoin_cli_a_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
libbitcoin_cli_a_SOURCES = \
  rpc/client.cpp \
  $(BITCOIN_CORE_H)

nodist_libbitcoin_util_a_SOURCES = $(srcdir)/obj/build.h
#

# btcud binary #
btcud_SOURCES = btcud.cpp
btcud_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
btcud_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
btcud_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
btcud_SOURCES += btcud-res.rc
endif
btcud_LDADD = \
  $(LIBBITCOIN_SERVER) \
  $(LIBBITCOIN_WALLET) \
  $(LIBBITCOIN_COMMON) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_ZEROCOIN) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_ZMQ) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBFF) \
  $(LIBSECP256K1)

btcud_LDADD += $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(MINIUPNPC_LIBS) $(EVENT_PTHREADS_LIBS) $(EVENT_LIBS) $(ZMQ_LIBS)

# btcu-cli binary #
btcu_cli_SOURCES = btcu-cli.cpp
btcu_cli_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(EVENT_CFLAGS)
btcu_cli_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
btcu_cli_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
btcu_cli_SOURCES += btcu-cli-res.rc
endif

btcu_cli_LDADD = \
  $(LIBBITCOIN_CLI) \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBSECP256K1)

btcu_cli_LDADD += $(BOOST_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(EVENT_LIBS)
#

# btcu-tx binary #
btcu_tx_SOURCES = btcu-tx.cpp
btcu_tx_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES)
btcu_tx_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
btcu_tx_LDFLAGS = $(RELDFLAGS) $(AM_LDFLAGS) $(LIBTOOL_APP_LDFLAGS)

if TARGET_WINDOWS
btcu_tx_SOURCES += btcu-tx-res.rc
endif

btcu_tx_LDADD = \
  $(LIBUNIVALUE) \
  $(LIBBITCOIN_SERVER) \
  $(LIBBITCOIN_COMMON) \
  $(LIBBITCOIN_ZEROCOIN) \
  $(LIBBITCOIN_UTIL) \
  $(LIBBITCOIN_CRYPTO) \
  $(LIBLEVELDB) \
  $(LIBLEVELDB_SSE42) \
  $(LIBMEMENV) \
  $(LIBCRYPTOPP) \
  $(LIBSECP256K1)

btcu_tx_LDADD += $(BOOST_LIBS) $(CRYPTO_LIBS)
#

# bitcoinconsensus library #
if BUILD_BITCOIN_LIBS
include_HEADERS = script/bitcoinconsensus.h
libbitcoinconsensus_la_SOURCES = \
  allocators.cpp \
  primitives/transaction.cpp \
  crypto/hmac_sha512.cpp \
  crypto/scrypt.cpp \
  crypto/sha1.cpp \
  crypto/sha256.cpp \
  crypto/sha512.cpp \
  crypto/ripemd160.cpp \
  hash.cpp \
  pubkey.cpp \
  script/script.cpp \
  script/interpreter.cpp \
  script/bitcoinconsensus.cpp \
  uint256.cpp \
  utilstrencodings.cpp

if GLIBC_BACK_COMPAT
  libbitcoinconsensus_la_SOURCES += compat/glibc_compat.cpp
endif

libbitcoinconsensus_la_LDFLAGS = $(AM_LDFLAGS) -no-undefined $(RELDFLAGS)
libbitcoinconsensus_la_LIBADD = $(LIBSECP256K1)
libbitcoinconsensus_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(builddir)/obj -I$(srcdir)/secp256k1/include -DBUILD_BITCOIN_INTERNAL
libbitcoinconsensus_la_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)

endif
#

CLEANFILES = $(EXTRA_LIBRARIES)

CLEANFILES += *.gcda *.gcno
CLEANFILES += compat/*.gcda compat/*.gcno
CLEANFILES += crypto/*.gcda crypto/*.gcno
CLEANFILES += libzerocoin/*.gcda libzerocoin/*.gcno
CLEANFILES += primitives/*.gcda primitives/*.gcno
CLEANFILES += rpc/*.gcda rpc/*.gcno
CLEANFILES += script/*.gcda script/*.gcno
CLEANFILES += support/*.gcda support/*.gcno
CLEANFILES += univalue/*.gcda univalue/*.gcno
CLEANFILES += wallet/*.gcda wallet/*.gcno
CLEANFILES += wallet/test/*.gcda wallet/test/*.gcno
CLEANFILES += leasing/*.gcda leasing/*.gcno
CLEANFILES += zmq/*.gcda zmq/*.gcno
CLEANFILES += zbtcu/*.gcda zbtcu/*.gcno
CLEANFILES += obj/build.h

EXTRA_DIST =


config/btcu-config.h: config/stamp-h1
	@$(MAKE) -C $(top_builddir) $(subdir)/$(@)
config/stamp-h1: $(top_srcdir)/$(subdir)/config/btcu-config.h.in $(top_builddir)/config.status
	$(AM_V_at)$(MAKE) -C $(top_builddir) $(subdir)/$(@)
$(top_srcdir)/$(subdir)/config/btcu-config.h.in:  $(am__configure_deps)
	$(AM_V_at)$(MAKE) -C $(top_srcdir) $(subdir)/config/btcu-config.h.in

clean-local:
	-$(MAKE) -C secp256k1 clean
	-$(MAKE) -C univalue clean
	-rm -f leveldb/*/*.gcda leveldb/*/*.gcno leveldb/helpers/memenv/*.gcda leveldb/helpers/memenv/*.gcno
	-rm -f config.h
	-rm -rf test/__pycache__

.rc.o:
	@test -f $(WINDRES)
	## FIXME: How to get the appropriate modulename_CPPFLAGS in here?
	$(AM_V_GEN) $(WINDRES) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) -DWINDRES_PREPROC -i $< -o $@

check-symbols: $(bin_PROGRAMS)
if GLIBC_BACK_COMPAT
	@echo "Checking glibc back compat..."
	$(AM_V_at) READELF=$(READELF) CPPFILT=$(CPPFILT) $(PYTHON) $(top_srcdir)/contrib/devtools/symbol-check.py < $(bin_PROGRAMS)
endif

check-security: $(bin_PROGRAMS)
if HARDEN
	@echo "Checking binary security..."
	$(AM_V_at) READELF=$(READELF) OBJDUMP=$(OBJDUMP) $(PYTHON) $(top_srcdir)/contrib/devtools/security-check.py < $(bin_PROGRAMS)
endif

%.pb.cc %.pb.h: %.proto
	@test -f $(PROTOC)
	$(AM_V_GEN) $(PROTOC) --cpp_out=$(@D) --proto_path=$(<D) $<

if EMBEDDED_LEVELDB
include Makefile.leveldb.include
endif

include Makefile.cryptopp.include

include Makefile.libff.include

if ENABLE_TESTS
include Makefile.test.include
endif

if ENABLE_QT
include Makefile.qt.include
endif

if ENABLE_QT_TESTS
include Makefile.qttest.include
endif
