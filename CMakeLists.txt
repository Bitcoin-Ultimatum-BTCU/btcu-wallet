cmake_minimum_required(VERSION 3.10)

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

set(HUNTER_CONFIGURATION_TYPES Release CACHE STRING "Hunter configuration types")
set(HUNTER_JOBS_NUMBER 4 CACHE STRING "Hunter jobs number")
set(HUNTER_CACHE_SERVERS "https://github.com/ethereum/hunter-cache" CACHE STRING "Hunter cache servers")
set(HUNTER_PASSWORDS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Hunter/passwords.cmake CACHE STRING "Hunter password path")

# In CI builds upload the binaries if the GITHUB_USER_PASSWORD was decrypted
# (only for branches and internal PRs).
if("$ENV{CI}" AND NOT "$ENV{GITHUB_USER_PASSWORD}" STREQUAL "")
    set(run_upload YES)
else()
    set(run_upload NO)
endif()
option(HUNTER_RUN_UPLOAD "Upload binaries to the cache server" ${run_upload})

include(HunterGate)
HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.23.44.tar.gz"
        SHA1 "c4cfcc0cd39fdae8de08f6205b7f34cab4a7ba79"
        LOCAL
)

project(BTCU)


set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS program_options filesystem system thread context fiber chrono log REQUIRED)
# add_definitions(-DBOOST_LOG_DYN_LINK)
add_definitions(-DETH_FATDB=1)

message(STATUS Boost_LIBRARIES:)
message (STATUS ${Boost_LIBRARIES})
message(STATUS BOOST_INCLUDEDIR:)
message(STATUS ${BOOST_INCLUDEDIR})

hunter_add_package(jsoncpp)
find_package(jsoncpp CONFIG REQUIRED)

hunter_add_package(cryptopp)
find_package(cryptopp CONFIG REQUIRED)

hunter_add_package(libjson-rpc-cpp)
find_package(libjson-rpc-cpp CONFIG REQUIRED)

hunter_add_package(libscrypt)
find_package(libscrypt CONFIG REQUIRED)

hunter_add_package(ethash)
find_package(ethash CONFIG REQUIRED)

hunter_add_package(Snappy)
find_package(Snappy CONFIG REQUIRED)

find_library(ROCKSDB_LIBRARY rocksdb)

    set(BDB_VER "18.1.32")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/contrib/cmake")
set(CMAKE_CXX_STANDARD 11)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)


if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message(FATAL_ERROR "Native Windows CMake is not supported yet, use WSL instead")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "Compiling on macOS")
    set(ENV{target} "Mac")
    list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/qt5)
    list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/openssl)
    list(APPEND CMAKE_PREFIX_PATH /usr/local/Cellar/berkeley-db@4)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl/")
    set(ENV{CPPFLAGS} "-I${OPENSSL_ROOT_DIR}/include")
    set(ENV{LDFLAGS} "-L${OPENSSL_ROOT_DIR}/lib")
    set(BerkeleyDB_ROOT_DIR "/usr/local/Cellar/berkeley-db@4/${BDB_VER}/")
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_NO_BOOST_CMAKE ON)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(ENV{target} "Linux")
    file(READ "/proc/version" _SYS_VERSION)
    string(REGEX MATCH "Microsoft" _SYSTEM_VERSION_PARSED "${_SYS_VERSION}")
    if(${_SYSTEM_VERSION_PARSED} MATCHES "Microsoft")
        message(STATUS "Compiling on WSL")
        set(ENV{DISPLAY} ":0")
        set(ENV{LIBGL_ALWAYS_INDIRECT} 1)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY /home/$ENV{USER}/btcu-run)
        message(WARNING "WSL Runtime support requires VcXsrv to be installed and running")
    else()
        message(STATUS "Compiling on Linux")
    endif()
    list(APPEND CMAKE_PREFIX_PATH /usr/lib/x86_64-linux-gnu/cmake/Qt5)
    set(Qt5core_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt5/QtCore")
    find_path(ENDIAN_INCLUDES endian.h)
    if(ENDIAN_INCLUDES)
        message(STATUS "Found endian.h: ${ENDIAN_INCLUDES}")
    endif()
    #add_definitions("-D__BYTE_ORDER -D__LITTLE_ENDIAN -D__GLIBC__ -DHAVE_DECL_BSWAP_16=0 -DHAVE_DECL_BSWAP_32=0 -DHAVE_DECL_BSWAP_64=0")
endif()

# Find Dependencies
find_package(BerkeleyDB ${BDB_VER} REQUIRED)
if(BerkeleyDB_FOUND)
    if(NOT ${BerkeleyDB_VERSION} MATCHES "${BDB_VER}")
        message(WARNING "BerkeleyDB version other than ${BDB_VER} found!")
        set(BDB_CONFIGURE_FLAGS "--with-incompatible-bdb")
    endif()
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    message(STATUS "Found OpenSSL (${OPENSSL_VERSION}): ${OPENSSL_LIBRARIES}")
    if(OPENSSL_VERSION VERSION_GREATER_EQUAL 1.1)
        message(STATUS "Found unsupported OpenSSL version!")
        set(SSL_CONFIGURE_FLAGS "--with-unsupported-ssl")
    endif()
endif()

find_package(LibEvent REQUIRED)

find_package(GMP)
if(GMP_FOUND)
else()
    message(WARNING "GMP not found, falling back to OpenSSL for bignum!")
    set(BIGNUM_CONFIGURE_FLAGS "--with-zerocoin-bignum=openssl")
endif()

find_package(ZMQ)
find_package(Miniupnp)

# run autogen.sh if missing header files from configure on Linux/Mac
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configure")
else()
    execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/autogen.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

# run configure if btcu_config.h doesn't exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/config/btcu-config.h")
else()
    execute_process(
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/configure ${CONFIGSITE} ${BDB_CONFIGURE_FLAGS} ${BIGNUM_CONFIGURE_FLAGS} ${SSL_CONFIGURE_FLAGS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
add_definitions(-DHAVE_CONFIG_H)

ExternalProject_Add (
        libunivalue
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/univalue
        CONFIGURE_COMMAND ""
        BUILD_COMMAND $(MAKE)
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND ""
        )

ExternalProject_Add (
        libsecp256k1
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/secp256k1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND $(MAKE)
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND ""
        )

add_subdirectory(src/libff)

set(libleveldb_a_headers
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/port/atomic_pointer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/port/port_example.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/port/port_posix.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/port/win/stdint.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/port/port.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/port/port_win.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/port/thread_annotations.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/db.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/options.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/comparator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/filter_policy.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/slice.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/table_builder.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/env.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/c.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/iterator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/cache.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/dumpfile.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/table.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/write_batch.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include/leveldb/status.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/log_format.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/memtable.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/version_set.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/write_batch_internal.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/filename.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/version_edit.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/dbformat.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/builder.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/log_writer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/db_iter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/skiplist.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/db_impl.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/table_cache.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/snapshot.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/log_reader.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/filter_block.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/block_builder.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/block.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/two_level_iterator.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/merger.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/format.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/iterator_wrapper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/crc32c.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/env_posix_test_helper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/arena.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/random.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/posix_logger.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/hash.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/histogram.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/coding.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/testutil.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/mutexlock.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/logging.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/testharness.h
        )

set(libleveldb_a_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/builder.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/c.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/dbformat.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/db_impl.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/db_iter.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/dumpfile.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/filename.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/log_reader.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/log_writer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/memtable.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/repair.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/table_cache.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/version_edit.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/version_set.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/db/write_batch.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/block_builder.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/block.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/filter_block.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/format.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/iterator.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/merger.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/table_builder.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/table.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/table/two_level_iterator.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/arena.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/bloom.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/cache.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/coding.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/comparator.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/crc32c.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/env.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/env_posix.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/filter_policy.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/hash.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/histogram.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/logging.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/options.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/status.cc
        )

set(libleveldb_cpp_flags)
if($ENV{target} MATCHES "Windows")
        set(libleveldb_cpp_flags -DOS_WINDOWS -DLEVELDB_PLATFORM_WINDOWS -DWINVER=0x0500 -D__USE_MINGW_ANSI_STDIO=1)
        set(libleveldb_a_sources ${libleveldb_a_sources} ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/util/env_win.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/port/port_win.cc)
else()
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(libleveldb_cpp_flags -DOS_MACOSX -DLEVELDB_PLATFORM_POSIX)
    else()
        set(libleveldb_cpp_flags -DOS_LINUX -DLEVELDB_PLATFORM_POSIX)
    endif()
    set(libleveldb_a_sources ${libleveldb_a_sources} ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/port/port_posix.cc)
endif()

add_library(leveldb STATIC ${libleveldb_a_headers} ${libleveldb_a_sources})
target_compile_definitions(leveldb PUBLIC ${libleveldb_cpp_flags} -DLEVELDB_ATOMIC_PRESENT -D__STDC_LIMIT_MACROS)
target_include_directories(leveldb PUBLIC ${ENDIAN_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers/memenv
        )

set(libmemenv_a_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers/memenv/memenv.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers/memenv/memenv.h
        )
add_library(memenv STATIC ${libmemenv_a_sources})
target_compile_definitions(memenv PUBLIC ${libleveldb_cpp_flags})
target_include_directories(memenv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers/memenv
        )

set(libleveldb_sse42_a_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/port/port_posix_sse.cc)
add_library(leveldb_sse42 ${libleveldb_sse42_a_sources})
target_include_directories(leveldb_sse42 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers/memenv
        )

file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB CRYPTO_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/*.h)
file(GLOB PRIMITIVE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/primitives/*.h)
file(GLOB ZMQ_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/zmq/*.h)
file(GLOB SCRIPT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/script/*.h)
file(GLOB RPC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/rpc/*.h)
file(GLOB COMPAT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/compat/*.h)

source_group("BitcoinHeaders" FILES
        ${HEADERS}
        ${CRYPTO_HEADERS}
        ${PRIMITIVE_HEADERS}
        ${ZMQ_HEADERS}
        ${SCRIPT_HEADERS}
        ${RPC_HEADERS}
        ${COMPAT_HEADERS}
        ./src/support/cleanse.h
        )

set(SERVER_SOURCES
        ./src/addrman.cpp
        ./src/alert.cpp
        ./src/bloom.cpp
        ./src/blocksignature.cpp
        ./src/chain.cpp
        ./src/checkpoints.cpp
        ./src/httprpc.cpp
        ./src/httpserver.cpp
        ./src/init.cpp
        ./src/leveldbwrapper.cpp
        ./src/main.cpp
        ./src/contract.cpp
        ./src/merkleblock.cpp
        ./src/miner.cpp
        ./src/net.cpp
        ./src/noui.cpp
        ./src/pow.cpp
        ./src/rest.cpp
        ./src/rpc/blockchain.cpp
        ./src/rpc/masternode.cpp
        ./src/rpc/budget.cpp
        ./src/rpc/mining.cpp
        ./src/rpc/misc.cpp
        ./src/rpc/net.cpp
        ./src/rpc/rawtransaction.cpp
        ./src/rpc/server.cpp
        ./src/rpc/validators.cpp
        ./src/masternode-validators.cpp
        ./src/script/sigcache.cpp
        ./src/sporkdb.cpp
        ./src/timedata.cpp
        ./src/torcontrol.cpp
        ./src/txdb.cpp
        ./src/txmempool.cpp
        ./src/validationinterface.cpp
        ./src/zbtcuchain.cpp
        ./src/cpp-ethereum/ethash/lib/ethash/keccak.c
        ./src/cpp-ethereum/ethash/lib/ethash/ethash.cpp
        ./src/cpp-ethereum/ethash/lib/ethash/keccakf1600.c
        ./src/cpp-ethereum/ethash/lib/ethash/primes.c
        ./src/cpp-ethereum/libdevcore/CommonData.cpp
        ./src/qtum/qtumDGP.cpp
        ./src/qtum/qtumstate.cpp
        #./src/cpp-ethereum/libethereum/State.cpp

        ./src/cpp-ethereum/aleth/buildinfo.c
        ./src/cpp-ethereum/evmc/lib/instructions/instruction_metrics.c
        ./src/cpp-ethereum/evmc/lib/instructions/instruction_names.c
        ./src/cpp-ethereum/libdevcore/Address.cpp
        ./src/cpp-ethereum/libdevcore/Base64.cpp
        ./src/cpp-ethereum/libdevcore/Common.cpp
        ./src/cpp-ethereum/libdevcore/CommonData.cpp
        ./src/cpp-ethereum/libdevcore/CommonIO.cpp
        ./src/cpp-ethereum/libdevcore/CommonJS.cpp
        ./src/cpp-ethereum/libdevcore/FileSystem.cpp
        ./src/cpp-ethereum/libdevcore/FixedHash.cpp
        ./src/cpp-ethereum/libdevcore/Guards.cpp
        ./src/cpp-ethereum/libdevcrypto/Hash.cpp
        ./src/cpp-ethereum/libdevcore/Log.cpp
        ./src/cpp-ethereum/libdevcore/LevelDB.cpp
        ./src/cpp-ethereum/libdevcore/MemoryDB.cpp
        ./src/cpp-ethereum/libdevcore/OverlayDB.cpp
        ./src/cpp-ethereum/libdevcore/StateCacheDB.cpp
        ./src/cpp-ethereum/libdevcore/RLP.cpp
        ./src/cpp-ethereum/libdevcore/SHA3.cpp
        ./src/cpp-ethereum/libdevcore/TransientDirectory.cpp
        ./src/cpp-ethereum/libdevcore/TrieCommon.cpp
        ./src/cpp-ethereum/libdevcore/Worker.cpp
        ./src/cpp-ethereum/libdevcore/DBFactory.cpp
        ./src/cpp-ethereum/libdevcore/JsonUtils.cpp
        ./src/cpp-ethereum/libevm/EVMC.cpp
        ./src/cpp-ethereum/libevm/ExtVMFace.cpp
        ./src/cpp-ethereum/libaleth-interpreter/VM.cpp
        ./src/cpp-ethereum/libaleth-interpreter/VMOpt.cpp
        ./src/cpp-ethereum/libaleth-interpreter/VMCalls.cpp
        ./src/cpp-ethereum/libevm/VMFactory.cpp
        ./src/cpp-ethereum/libevm/Instruction.cpp
        ./src/cpp-ethereum/libevm/LegacyVM.cpp
        ./src/cpp-ethereum/libevm/LegacyVMCalls.cpp
        ./src/cpp-ethereum/libevm/LegacyVMOpt.cpp
        ./src/cpp-ethereum/libethereum/ImportPerformanceLogger.cpp
        ./src/cpp-ethereum/libethereum/Account.cpp
        ./src/cpp-ethereum/libethereum/GasPricer.cpp
        ./src/cpp-ethereum/libethereum/State.cpp
        ./src/cpp-ethereum/libethcore/ABI.cpp
        ./src/cpp-ethereum/libethcore/ChainOperationParams.cpp
        ./src/cpp-ethereum/libethcore/Common.cpp
        ./src/cpp-ethereum/libethcore/Precompiled.cpp
        ./src/cpp-ethereum/libethcore/TransactionBase.cpp
        ./src/cpp-ethereum/libdevcrypto/Common.cpp
        ./src/cpp-ethereum/libdevcrypto/CryptoPP.cpp
        ./src/cpp-ethereum/libdevcrypto/AES.cpp
        ./src/cpp-ethereum/libdevcrypto/LibSnark.cpp
        ./src/cpp-ethereum/libethereum/ChainParams.cpp
        ./src/cpp-ethereum/libethereum/Transaction.cpp
        ./src/cpp-ethereum/libethereum/Executive.cpp
        ./src/cpp-ethereum/libethereum/ExtVM.cpp
        ./src/cpp-ethereum/libethereum/Block.cpp
        ./src/cpp-ethereum/libethereum/BlockChain.cpp
        ./src/cpp-ethereum/libethereum/BlockDetails.cpp
        ./src/cpp-ethereum/libethereum/TransactionQueue.cpp
        ./src/cpp-ethereum/libethereum/BlockQueue.cpp
        ./src/cpp-ethereum/libethcore/BlockHeader.cpp
        ./src/cpp-ethereum/libdevcore/RLP.cpp
        ./src/cpp-ethereum/libethereum/TransactionReceipt.cpp
        ./src/cpp-ethereum/libethcore/SealEngine.cpp
        ./src/cpp-ethereum/libdevcore/TrieHash.cpp
        ./src/cpp-ethereum/libethereum/GenesisInfo.cpp

        ./src/cpp-ethereum/libethereum/ValidationSchemes.cpp
        ./src/cpp-ethereum/libethcore/LogEntry.cpp
        ./src/cpp-ethereum/ethash/lib/ethash/ethash.cpp
        ./src/cpp-ethereum/ethash/lib/ethash/keccak.c
        ./src/cpp-ethereum/ethash/lib/ethash/keccakf800.c
        ./src/cpp-ethereum/ethash/lib/ethash/keccakf1600.c
        ./src/cpp-ethereum/ethash/lib/ethash/managed.cpp
        ./src/cpp-ethereum/ethash/lib/ethash/primes.c
        ./src/cpp-ethereum/ethash/lib/ethash/progpow.cpp
        ./src/cpp-ethereum/evmc/lib/loader/loader.c
        ./src/cpp-ethereum/libdevcore/RocksDB.cpp
        ./src/validation.cpp
        ./src/consensus/consensus.cpp
        ./src/zbtcuchain.cpp
        )


add_library(SERVER_A STATIC ${BitcoinHeaders} ${SERVER_SOURCES})
if(MINIUPNP_FOUND)
    target_compile_definitions(SERVER_A PUBLIC "-DSTATICLIB -DMINIUPNP_STATICLIB")
endif()
target_include_directories(SERVER_A PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers/memenv
    ${ZMQ_INCLUDE_DIR} ${LIBEVENT_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${BerkeleyDB_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/ethash/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/evmc/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qtum
    ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/libdevcore
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/utils/libscrypt
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libff/libff
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libff
    )
target_link_libraries(SERVER_A ${Boost_LIBRARIES} cryptopp-static libscrypt::scrypt jsoncpp_lib_static
        ${ROCKSDB_LIBRARY}
        ${CMAKE_DL_LIBS} Threads::Threads ff Snappy::snappy)

if(ZMQ_FOUND)
    set(ZMQ_SOURCES
        ./src/zmq/zmqabstractnotifier.cpp
        ./src/zmq/zmqnotificationinterface.cpp
        ./src/zmq/zmqpublishnotifier.cpp
    )
    add_library(ZMQ_A STATIC ${BitcoinHeaders} ${ZMQ_SOURCES} ${ZMQ_LIB})
    target_include_directories(ZMQ_A PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${ZMQ_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})
    target_compile_definitions(ZMQ_A PUBLIC "-DZMQ_STATIC")
endif()

set(WALLET_SOURCES
        ./src/activemasternode.cpp
        ./src/bip38.cpp
        ./src/denomination_functions.cpp
        ./src/obfuscation.cpp
        ./src/obfuscation-relay.cpp
        ./src/wallet/db.cpp
        ./src/addressbook.cpp
        ./src/crypter.cpp
        ./src/swifttx.cpp
        ./src/masternode.cpp
        ./src/masternode-budget.cpp
        ./src/masternode-payments.cpp
        ./src/masternode-sync.cpp
        ./src/masternodeconfig.cpp
        ./src/masternodeman.cpp
        ./src/messagesigner.cpp
        ./src/zbtcu/mintpool.cpp
        ./src/wallet/rpcdump.cpp
        ./src/zbtcu/deterministicmint.cpp
        ./src/zbtcu/zerocoin.cpp
        ./src/wallet/rpcwallet.cpp
        ./src/kernel.cpp
        ./src/wallet/wallet.cpp
        ./src/wallet/wallet_zerocoin.cpp
        ./src/wallet/wallet_ismine.cpp
        ./src/wallet/walletdb.cpp
        ./src/leasing/leasingmanager.cpp
        ./src/leasing/leasing_tx_verify.cpp
        ./src/zbtcu/zbtcuwallet.cpp
        ./src/zbtcu/zbtcutracker.cpp
        ./src/zbtcu/zbtcumodule.cpp
        ./src/zbtcu/zpos.cpp
        ./src/stakeinput.cpp
        )
add_library(WALLET_A STATIC ${BitcoinHeaders} ${WALLET_SOURCES})
target_include_directories(WALLET_A PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/ethash/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/evmc/include
        ${OPENSSL_INCLUDE_DIR}
        ${BerkeleyDB_INCLUDE_DIRS}
        )

set(BITCOIN_CRYPTO_SOURCES
        ./src/crypto/sha1.cpp
        ./src/crypto/sha256.cpp
        ./src/crypto/sha512.cpp
        ./src/crypto/chacha20.cpp
        ./src/crypto/hmac_sha256.cpp
        ./src/crypto/rfc6979_hmac_sha256.cpp
        ./src/crypto/hmac_sha512.cpp
        ./src/crypto/scrypt.cpp
        ./src/crypto/ripemd160.cpp
        ./src/crypto/aes_helper.c
        ./src/crypto/blake.c
        ./src/crypto/bmw.c
        ./src/crypto/groestl.c
        ./src/crypto/jh.c
        ./src/crypto/keccak.c
        ./src/crypto/skein.c
        )
add_library(BITCOIN_CRYPTO_A STATIC ${BITCOIN_CRYPTO_SOURCES})
target_include_directories(BITCOIN_CRYPTO_A PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${OPENSSL_INCLUDE_DIR})

set(ZEROCOIN_SOURCES
        ./src/libzerocoin/bignum.cpp
        ./src/libzerocoin/Accumulator.cpp
        ./src/libzerocoin/Coin.cpp
        ./src/libzerocoin/CoinRandomnessSchnorrSignature.cpp
        ./src/libzerocoin/Denominations.cpp
        ./src/libzerocoin/CoinSpend.cpp
        ./src/libzerocoin/ParamGeneration.cpp
        ./src/libzerocoin/Params.cpp
        )
if(GMP_FOUND)
    list(APPEND ZEROCOIN_SOURCES ./src/libzerocoin/bignum_gmp.cpp)
else()
    list(APPEND ZEROCOIN_SOURCES ./src/libzerocoin/bignum_openssl.cpp)
endif()
add_library(ZEROCOIN_A STATIC ${ZEROCOIN_SOURCES})
target_include_directories(ZEROCOIN_A PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${OPENSSL_INCLUDE_DIR}
        )

set(COMMON_SOURCES
        ./src/allocators.cpp
        ./src/amount.cpp
        ./src/base58.cpp
        ./src/bech32.cpp
        ./src/bip38.cpp
        ./src/chainparams.cpp
        ./src/coins.cpp
        ./src/compressor.cpp
        ./src/consensus/merkle.cpp
        ./src/consensus/tx_verify.cpp
        ./src/consensus/validator_tx_verify.cpp
        ./src/primitives/block.cpp
        ./src/zbtcu/deterministicmint.cpp
        ./src/primitives/transaction.cpp
        ./src/zbtcu/zerocoin.cpp
        ./src/core_read.cpp
        ./src/core_write.cpp
        ./src/hash.cpp
        ./src/invalid.cpp
        ./src/key.cpp
        ./src/btcu_address.cpp
        ./src/keystore.cpp
        ./src/netbase.cpp
        ./src/protocol.cpp
        ./src/pubkey.cpp
        ./src/scheduler.cpp
        ./src/script/interpreter.cpp
        ./src/script/script.cpp
        ./src/script/sign.cpp
        ./src/script/standard.cpp
        ./src/script/script_error.cpp
        ./src/spork.cpp
        ./src/sporkdb.cpp
        ./src/validators_state.cpp
        ./src/validators_voting.cpp

        ./src/cpp-ethereum/libethashseal/Ethash.cpp
        ./src/cpp-ethereum/libethashseal/EthashCPUMiner.cpp
        ./src/cpp-ethereum/libethashseal/EthashProofOfWork.cpp

        ./src/cpp-ethereum/libethashseal/GenesisInfo.cpp
        )

add_library(COMMON_A STATIC ${BitcoinHeaders} ${COMMON_SOURCES})
target_include_directories(COMMON_A PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
        ${OPENSSL_INCLUDE_DIR}
        ${BerkeleyDB_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/ethash/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/evmc/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/qtum
        )

set(UTIL_SOURCES
        ./src/allocators.cpp
        ./src/compat/strnlen.cpp
        ./src/compat/glibc_sanity.cpp
        ./src/compat/glibcxx_sanity.cpp
        ./src/chainparamsbase.cpp
        ./src/clientversion.cpp
        ./src/random.cpp
        ./src/rpc/protocol.cpp
        ./src/sync.cpp
        ./src/uint256.cpp
        ./src/util.cpp
        ./src/utilstrencodings.cpp
        ./src/utilmoneystr.cpp
        ./src/utiltime.cpp
        ./src/support/cleanse.cpp
        ./src/util/string.cpp
        )
add_library(UTIL_A STATIC ${BitcoinHeaders} ${UTIL_SOURCES})
target_include_directories(UTIL_A PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
        ${OPENSSL_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/
        )

set(CLI_A_SOURCES ./src/rpc/client.cpp)
add_library(CLI_A STATIC ${BitcoinHeaders} ${CLI_A_SOURCES})
target_include_directories(CLI_A PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
        )

link_directories(
        ${CMAKE_SOURCE_DIR}/src/univalue/.libs
        ${CMAKE_SOURCE_DIR}/src/secp256k1/.libs
        )

set(btcu-cli_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcu-cli.cpp)
if($ENV{target} MATCHES "Windows")
    list(APPEND btcu-cli_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcu-cli-res.rc)
endif()
add_executable(btcu-cli ${btcu-cli_SOURCES})
add_dependencies(btcu-cli libunivalue)
target_link_libraries(btcu-cli
        CLI_A
        univalue
        UTIL_A
        BITCOIN_CRYPTO_A
        ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${LIBEVENT_LIB}
	Threads::Threads
        )
if($ENV{target} MATCHES "Windows")
    target_link_libraries(btcu-cli ${WINDOWS_LDADD})
endif()

set(btcu-tx_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcu-tx.cpp)
if($ENV{target} MATCHES "Windows")
    list(APPEND btcu-tx_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcu-tx-res.rc)
endif()
add_executable(btcu-tx ${btcu-tx_SOURCES})
add_dependencies(btcu-tx libunivalue libsecp256k1)
target_link_libraries(btcu-tx
        univalue
        COMMON_A
        SERVER_A
        ZEROCOIN_A
        UTIL_A
        BITCOIN_CRYPTO_A
        secp256k1
        ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${LIBEVENT_LIB}
        )
if($ENV{target} MATCHES "Windows")
    target_link_libraries(btcu-tx ${WINDOWS_LDADD})
endif()
if(GMP_FOUND)
    target_link_libraries(btcu-tx ${GMP_LIBRARY})
endif()

link_directories(${CMAKE_BINARY_DIR})

set(btcud_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcud.cpp)
if($ENV{target} MATCHES "Windows")
    list(APPEND btcud_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcud-res.rc)
endif()
add_executable(btcud ${btcud_SOURCES} ${BitcoinHeaders})
add_dependencies(btcud libunivalue libsecp256k1 leveldb leveldb_sse42 memenv)
target_include_directories(btcud PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers/memenv
        ${LIBEVENT_INCLUDE_DIR})
target_link_libraries(btcud
        Threads::Threads
        SERVER_A
        WALLET_A
        univalue
        COMMON_A
        ZEROCOIN_A
        UTIL_A
        BITCOIN_CRYPTO_A
        ${CMAKE_BINARY_DIR}/libleveldb.a
        ${CMAKE_BINARY_DIR}/libleveldb_sse42.a
        ${CMAKE_BINARY_DIR}/libmemenv.a
        secp256k1
        ${BerkeleyDB_LIBRARIES} ${OPENSSL_LIBRARIES} ${Boost_LIBRARIES} ${LIBEVENT_LIB}
        )

if($ENV{target} MATCHES "Windows")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wstack-protector -fstack-protector-all -fPIE -pipe -O2 -pthread -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va -pie --static")
    target_link_libraries(btcud ${WINDOWS_LDADD})
endif()
if(GMP_FOUND)
    target_link_libraries(btcud ${GMP_LIBRARY})
    target_include_directories(btcud PUBLIC ${GMP_INCLUDE_DIR})
endif()
if(ZMQ_FOUND)
    target_link_libraries(btcud ZMQ_A ${ZMQ_LIB})
    target_include_directories(btcud PUBLIC ${ZMQ_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/)
endif()
if(MINIUPNP_FOUND)
    target_compile_definitions(btcud PUBLIC "-DSTATICLIB -DMINIUPNP_STATICLIB")
    target_link_libraries(btcud ${MINIUPNP_LIBRARY})
    target_include_directories(btcud PUBLIC ${MINIUPNP_INCLUDE_DIR})
endif()

add_subdirectory(src/qt)


set(BTCU_TEST_SOURCES
        ./src/test/test_btcu.cpp
        ./src/test/zerocoin_denomination_tests.cpp
        ./src/test/zerocoin_transactions_tests.cpp
        ./src/test/zerocoin_bignum_tests.cpp
        ./src/test/addrman_tests.cpp
        ./src/test/allocator_tests.cpp
        ./src/test/base32_tests.cpp
        ./src/test/base58_tests.cpp
        ./src/test/base64_tests.cpp
        ./src/test/budget_tests.cpp
        ./src/test/checkblock_tests.cpp
        ./src/test/Checkpoints_tests.cpp
        ./src/test/coins_tests.cpp
        ./src/test/compress_tests.cpp
        ./src/test/crypto_tests.cpp
        ./src/test/DoS_tests.cpp
        ./src/test/getarg_tests.cpp
        ./src/test/hash_tests.cpp
        ./src/test/key_tests.cpp
        ./src/test/main_tests.cpp
        ./src/test/mempool_tests.cpp
        ./src/test/merkle_tests.cpp
        ./src/test/mruset_tests.cpp
        ./src/test/multisig_tests.cpp
        ./src/test/netbase_tests.cpp
        ./src/test/pmt_tests.cpp
        ./src/test/random_tests.cpp
        ./src/test/reverselock_tests.cpp
        ./src/test/rpc_tests.cpp
        ./src/test/sanity_tests.cpp
        ./src/test/scheduler_tests.cpp
        ./src/test/script_P2SH_tests.cpp
        ./src/test/script_tests.cpp
        ./src/test/scriptnum_tests.cpp
        ./src/test/serialize_tests.cpp
        ./src/test/sighash_tests.cpp
        ./src/test/sigopcount_tests.cpp
        ./src/test/skiplist_tests.cpp
        ./src/test/timedata_tests.cpp
        ./src/test/torcontrol_tests.cpp
        ./src/test/transaction_tests.cpp
        ./src/test/uint256_tests.cpp
        ./src/test/univalue_tests.cpp
        ./src/test/util_tests.cpp

        # Wallet tests
        ./src/test/accounting_tests.cpp
        ./src/wallet/test/wallet_tests.cpp
        ./src/test/rpc_wallet_tests.cpp
    )

set(BTCU_JSON_TEST_FILES
        ./src/test/data/script_valid.json
        ./src/test/data/base58_keys_valid.json
        ./src/test/data/sig_canonical.json
        ./src/test/data/sig_noncanonical.json
        ./src/test/data/base58_encode_decode.json
        ./src/test/data/base58_keys_invalid.json
        ./src/test/data/script_invalid.json
        ./src/test/data/tx_invalid.json
        ./src/test/data/tx_valid.json
        ./src/test/data/sighash.json
    )

set(BTCU_RAW_TEST_FILES
        ./src/test/data/alertTests.raw
    )

find_program(HEXDUMP hexdump)
find_program(SED sed)

macro(BTCU_HEXDUMP SRC_FILE DST_NAMESPACE DST_LIST)
    get_filename_component(SRC_VAR_NAME ${SRC_FILE} NAME_WE)
    set(SRC_IN ${CMAKE_CURRENT_SOURCE_DIR}/${SRC_FILE})
    set(SRC_OUT ${CMAKE_CURRENT_BINARY_DIR}/${SRC_FILE}.h)
    set(SRC_TMP ${CMAKE_CURRENT_BINARY_DIR}/${SRC_FILE}.tmp)
    get_filename_component(SRC_OUT_DIR ${SRC_OUT} DIRECTORY)
    add_custom_command(
        OUTPUT
            ${SRC_OUT}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory ${SRC_OUT_DIR}
        COMMAND
            ${CMAKE_COMMAND} -E echo "namespace ${DST_NAMESPACE}" { > ${SRC_OUT}
        COMMAND
            ${CMAKE_COMMAND} -E echo "static unsigned const char ${SRC_VAR_NAME}[] = {" >> ${SRC_OUT}
        COMMAND
            ${HEXDUMP} -v -e '8/1 \"0x%02x, \"' -e '\"\\n\"' ${SRC_IN} > ${SRC_TMP}
        COMMAND
            ${SED} -e 's/0x\ \ ,//g' ${SRC_TMP} >> ${SRC_OUT}
        COMMAND
            ${CMAKE_COMMAND} -E echo "}\;}\;" >> ${SRC_OUT}
        MAIN_DEPENDENCY
            ${SRC_FILE}
    )
    list(APPEND ${DST_LIST} ${SRC_OUT})
endmacro(BTCU_HEXDUMP)

foreach(BTCU_JSON_FILE ${BTCU_JSON_TEST_FILES})
    BTCU_HEXDUMP(${BTCU_JSON_FILE} json_tests BTCU_HEX_TEST_FILES)
endforeach(BTCU_JSON_FILE)

foreach(BTCU_RAW_FILE ${BTCU_RAW_TEST_FILES})
    BTCU_HEXDUMP(${BTCU_RAW_FILE} alert_tests BTCU_HEX_TEST_FILES)
endforeach(BTCU_RAW_FILE)

list(GET BTCU_HEX_TEST_FILES 0 BTCU_HEX_DEST_FILE)
get_filename_component(BTCU_HEX_DESTDIR ${BTCU_HEX_DEST_FILE} DIRECTORY)
get_filename_component(BTCU_HEX_DESTDIR ${BTCU_HEX_DESTDIR} DIRECTORY)
add_custom_target(test_btcu_hex ALL DEPENDS ${BTCU_HEX_TEST_FILES})

add_executable(test_btcu ${BTCU_TEST_SOURCES})
target_compile_definitions(test_btcu PUBLIC -DBOOST_TEST_DYN_LINK)
add_dependencies(test_btcu test_btcu_hex libunivalue libsecp256k1 leveldb leveldb_sse42 memenv)
target_include_directories(test_btcu PUBLIC ${BTCU_HEX_DESTDIR} ./src/test)

target_link_libraries(test_btcu
        SERVER_A
        WALLET_A
        univalue
        COMMON_A
        ZEROCOIN_A
        UTIL_A
        CLI_A
        BITCOIN_CRYPTO_A
        ${CMAKE_BINARY_DIR}/libleveldb.a
        ${CMAKE_BINARY_DIR}/libleveldb_sse42.a
        ${CMAKE_BINARY_DIR}/libmemenv.a
        secp256k1
        ${BerkeleyDB_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${Boost_LIBRARIES}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${LIBEVENT_LIB}
    )

if(GMP_FOUND)
    target_link_libraries(test_btcu ${GMP_LIBRARY})
    target_include_directories(test_btcu PUBLIC ${GMP_INCLUDE_DIR})
endif()
if(ZMQ_FOUND)
    target_include_directories(test_btcu PUBLIC ${ZMQ_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/)
    target_link_libraries(test_btcu ZMQ_A ${ZMQ_LIB})
endif()
if(MINIUPNP_FOUND)
    target_compile_definitions(test_btcu PUBLIC "-DSTATICLIB -DMINIUPNP_STATICLIB")
    target_link_libraries(test_btcu ${MINIUPNP_LIBRARY})
    target_include_directories(test_btcu PUBLIC ${MINIUPNP_INCLUDE_DIR})
endif()
