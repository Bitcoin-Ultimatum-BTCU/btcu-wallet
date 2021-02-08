cmake_minimum_required(VERSION 3.14)

project (libff)

# Default to RelWithDebInfo configuration if no configuration is explicitly specified.
#if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
#  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type on single-configuration generators" FORCE)
#endif()

set(
  CURVE
  "BN128"
  CACHE
  STRING
  "Default curve: one of ALT_BN128, BN128, EDWARDS, MNT4, MNT6"
)

option(
  DEBUG
  "Enable debugging mode"
  OFF
)
option(
  LOWMEM
  "Limit the size of multi-exponentiation tables, for low-memory platforms"
  OFF
)
option(
  MULTICORE
  "Enable parallelized execution, using OpenMP"
  OFF
)
option(
  BINARY_OUTPUT
  "In serialization, output raw binary data (instead of decimal), which is smaller and faster."
  ON
)
option(
  MONTGOMERY_OUTPUT
  "Serialize Fp elements as their Montgomery representations (faster but not human-readable)"
  ON
)
option(
  USE_PT_COMPRESSION
  "Use point compression"
  ON
)
option(
  PROFILE_OP_COUNTS
  "Collect counts for field and curve operations"
  OFF
)
option(
  USE_MIXED_ADDITION
  "Convert each element of the key pair to affine coordinates"
  OFF
)

option(
  WITH_PROCPS
  "Use procps for memory profiling"
  OFF
)

option(
  CPPDEBUG
  "Enable debugging of C++ STL (does not imply DEBUG)"
  OFF
)

option(
  PERFORMANCE
  "Enable link-time and aggressive optimizations"
  OFF
)

option(
  USE_ASM
  "Use architecture-specific optimized assembly code"
  ON
)

option(
  IS_LIBFF_PARENT
  "Install submodule dependencies if caller originates from here"
  ON
)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
  # Common compilation flags and warning configuration
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wfatal-errors"
  )
  if("${MULTICORE}")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
  endif()
endif()

# You have to uncomment this in order to build libff from the current folder: 

# set( GMP_INCLUDE_DIR "/usr/local/opt/gmp/include" )
# set( GMP_LIBRARY "/usr/local/opt/gmp/lib" )
# set( OPENSSL_INCLUDE_DIR "/usr/local/opt/openssl/include/openssl" )
# set( OPENSSL_CRYPTO_LIBRARY "/usr/local/opt/openssl/lib" )
# set( OPENSSL_SSL_LIBRARY "/usr/local/opt/openssl/lib" 
# set(CMAKE_MODULE_PATH "../../cmake/scripts")
# include(BrewHelper)
# set(CMAKE_MODULE_PATH "../../contrib/cmake")
# ind_package(GMP REQUIRED)
# find_package(OpenSSL REQUIRED)
# set(CMAKE_REQUIRED_INCLUDES ${OPENSSL_INCLUDE_DIR})
# set(CMAKE_REQUIRED_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY})

if("${WITH_PROCPS}")
  include(FindPkgConfig)
  pkg_check_modules(
    PROCPS
    REQUIRED

    libprocps
  )
else()
  add_definitions(
    -DNO_PROCPS
  )
endif()

add_definitions(
  -DCURVE_${CURVE}
)

enable_testing()

if(${CURVE} STREQUAL "BN128")
  add_definitions(
    -DBN_SUPPORT_SNARK=1
  )
endif()

if("${DEBUG}")
  add_definitions(-DDEBUG=1)
endif()

if("${LOWMEM}")
  add_definitions(-DLOWMEM=1)
endif()

if("${MULTICORE}")
  add_definitions(-DMULTICORE=1)
endif()

if("${BINARY_OUTPUT}")
  add_definitions(-DBINARY_OUTPUT)
endif()

if("${MONTGOMERY_OUTPUT}")
  add_definitions(-DMONTGOMERY_OUTPUT)
endif()

if(NOT "${USE_PT_COMPRESSION}")
  add_definitions(-DNO_PT_COMPRESSION=1)
endif()

if("${PROFILE_OP_COUNTS}")
  add_definitions(-DPROFILE_OP_COUNTS=1)
endif()

if("${USE_MIXED_ADDITION}")
  add_definitions(-DUSE_MIXED_ADDITION=1)
endif()

if("${CPPDEBUG}")
  add_definitions(-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC)
endif()

if("${PERFORMANCE}")
  set(
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -flto -fuse-linker-plugin"
  )
  set(
    CMAKE_EXE_LINKER_FLAGS
    "${CMAKE_EXE_LINKER_FLAGS} -flto"
  )
endif()

if("${USE_ASM}")
  add_definitions(-DUSE_ASM)
endif()

# Configure CCache if available
#find_program(CCACHE_FOUND ccache)
#if(CCACHE_FOUND)
#        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
#        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
#endif(CCACHE_FOUND)

if ("${IS_LIBFF_PARENT}")
  find_program(
    MARKDOWN

    markdown_py
    DOC "Path to markdown_py binary"
  )
  if(MARKDOWN-NOTFOUND)
  else()
     add_custom_target(
       doc
       ${MARKDOWN} -f ${CMAKE_CURRENT_BINARY_DIR}/README.html -x toc -x extra --noisy ${CMAKE_CURRENT_SOURCE_DIR}/README.md
       WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
       COMMENT "Translating from markdown to HTML" VERBATIM
     )
  endif()

  # Add a `make check` target that builds and tests
  add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

  # Add a `make profile` target that builds and profiles
  add_custom_target(
    profile
    COMMAND ${CMAKE_COMMAND}
     -E echo 'Built target finished'
  )

  add_subdirectory(depends)
endif()
add_subdirectory(libff)
