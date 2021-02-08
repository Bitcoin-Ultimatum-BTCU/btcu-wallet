# Copyright (c) 2017 The Bitcoin developers

message(STATUS "Compiling on macOS")

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(TOOLCHAIN_PREFIX ${CMAKE_SYSTEM_PROCESSOR}-apple-darwin16)

# On OSX, we use clang by default.
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_COMPILER_TARGET ${TOOLCHAIN_PREFIX})
set(CMAKE_CXX_COMPILER_TARGET ${TOOLCHAIN_PREFIX})

set(OSX_MIN_VERSION 10.10)
set(OSX_SDK_VERSION 10.10)
# OSX_SDK_VERSION 10.10
# Note: don't use XCODE_VERSION, it's a cmake built-in variable !
set(SDK_XCODE_VERSION 11.3.1)
set(SDK_XCODE_BUILD_ID 11C505)
set(LD64_VERSION 530)

# On OSX we use various stuff from Apple's SDK.
# set(OSX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/depends/SDKs/Xcode-${SDK_XCODE_VERSION}-${SDK_XCODE_BUILD_ID}-extracted-SDK-with-libcxx-headers")
# set(CMAKE_OSX_SYSROOT "${OSX_SDK_PATH}")

set(CMAKE_OSX_DEPLOYMENT_TARGET "${OSX_MIN_VERSION}")
set(CMAKE_OSX_ARCHITECTURES x86_64)

# modify default behavior of FIND_XXX() commands to
# search for headers/libs in the target environment and
# search for programs in the build host environment
# set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# TODO: set downloaded SKD using
set(OSX_SDK_PATH "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${OSX_SDK_VERSION}.sdk")
set(CMAKE_OSX_SYSROOT "${OSX_SDK_PATH}")

# When cross-compiling for Darwin using Clang, -mlinker-version must be passed
# to ensure that modern linker features are enabled.
string(APPEND CMAKE_CXX_FLAGS_INIT " -stdlib=libc++ -mlinker-version=${LD64_VERSION}")

# Ensure we use an OSX specific version the binary manipulation tools.
find_program(CMAKE_AR ${TOOLCHAIN_PREFIX}-ar)
if(NOT CMAKE_AR)
    find_program(CMAKE_AR ar)
endif()
find_program(CMAKE_INSTALL_NAME_TOOL ${TOOLCHAIN_PREFIX}-install_name_tool)
if(NOT CMAKE_INSTALL_NAME_TOOL)
    find_program(CMAKE_AR install_name_tool)
endif()
find_program(CMAKE_LINKER ${TOOLCHAIN_PREFIX}-ld)
if(NOT CMAKE_LINKER)
    find_program(CMAKE_LINKER ld)
endif()
find_program(CMAKE_NM ${TOOLCHAIN_PREFIX}-nm)
if(NOT CMAKE_NM)
    find_program(CMAKE_NM nm)
endif()
find_program(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}-objcopy)
if(NOT CMAKE_OBJCOPY)
    find_program(CMAKE_OBJCOPY objcopy)
endif()
find_program(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}-objdump)
if(NOT CMAKE_OBJDUMP)
    find_program(CMAKE_OBJDUMP objdump)
endif()
find_program(CMAKE_OTOOL ${TOOLCHAIN_PREFIX}-otool)
if(NOT CMAKE_OTOOL)
    find_program(CMAKE_OTOOL otool)
endif()
find_program(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}-ranlib)
if(NOT CMAKE_RANLIB)
    find_program(CMAKE_RANLIB ranlib)
endif()
find_program(CMAKE_STRIP ${TOOLCHAIN_PREFIX}-strip)
if(NOT CMAKE_STRIP)
    find_program(CMAKE_STRIP strip)
endif()

# Path handling
# the RPATH to be used when installing
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)