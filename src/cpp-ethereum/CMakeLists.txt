# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2018 Aleth Autors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.

cmake_minimum_required(VERSION 3.14)

#if (NOT EXISTS ${CMAKE_SOURCE_DIR}/evmc/.git OR NOT EXISTS ${CMAKE_SOURCE_DIR}/cmake/cable/.git)
#    message(FATAL_ERROR "Git submodules not initialized, execute:\n  git submodule update --init")
#endif()

message(${CMAKE_CURRENT_LIST_DIR})
include(${CMAKE_SOURCE_DIR}/cmake/cable/bootstrap.cmake)
include(CableBuildInfo)
include(CableBuildType)
include(CableToolchains)
include(GNUInstallDirs)

cable_configure_toolchain(DEFAULT cxx11-pic)

set(ETH_CMAKE_DIR "${CMAKE_SOURCE_DIR}/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# Map current configuration to configurations of imported targets.
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release)
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)

project(cpp-ethereum)
set(PROJECT_VERSION 1.6.0-alpha.1)

cable_set_build_type(DEFAULT RelWithDebInfo CONFIGURATION_TYPES Debug;Release;RelWithDebInfo)

cable_add_buildinfo_library(PROJECT_NAME cpp-ethereum)

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY On)

include(EthCcache)
include(EthOptions)
include(EthCompilerSettings)
include(EthExecutableHelper)
include(EthDependencies)
include(EthUtils)

if (MSVC)
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_STATIC_LIBS ON)
endif()
set(CMAKE_CXX_STANDARD 11)

find_package(ethash CONFIG REQUIRED)

find_package(Threads)

set(UTILS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/utils")

configure_project()

if(TESTS)
    # Import binaryen library for testeth.
    # TODO: remove this once testeth uses binaryen as a CLI tool.
    include(ProjectBinaryen)
endif()

add_subdirectory(evmc)

#Global include path for all libs.
include_directories("${CMAKE_SOURCE_DIR}")

option(ALETH_INTERPRETER_SHARED "Build cpp-ethereum-interpreter as a shared library" OFF)
add_subdirectory(libaleth-interpreter)

#add_subdirectory(libdevcore)
add_subdirectory(libdevcrypto)
add_subdirectory(libp2p)

add_subdirectory(libethcore)
add_subdirectory(libevm)
add_subdirectory(libethereum)
add_subdirectory(libethashseal)

add_subdirectory(libwebthree)
add_subdirectory(libweb3jsonrpc)

#add_subdirectory(cpp-ethereum)

if (TOOLS)
    #add_subdirectory(cpp-ethereum-key)
    #add_subdirectory(cpp-ethereum-vm)
    #add_subdirectory(rlp)
    #add_subdirectory(cpp-ethereum-bootnode)
endif()

if (TESTS)
    #enable_testing()
   #add_subdirectory(test)
endif()


install(FILES ${PROJECT_BINARY_DIR}/cpp-ethereum/buildinfo.json DESTINATION ${CMAKE_INSTALL_DATADIR}/cpp-ethereum)


if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY FALSE)
include(CPack)
