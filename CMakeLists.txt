cmake_minimum_required(VERSION 3.14)
set(CMAKE_VERBOSE_MAKEFILE ON)

# the version of this package
set(PROJECT_VERSION 1.0.0.0)

project(btcu
	VERSION ${PROJECT_VERSION}
	DESCRIPTION "BTCU is a free open source peer-to-peer electronic cash system that is completely decentralized, without the need for a central server or trusted parties.  Users hold the crypto keys to their own money and transact directly with each other, with the help of a P2P network to check for double-spending."
	HOMEPAGE_URL "https://btcu.io/"
)

# Package information
set(PACKAGE_NAME "BTCU")
set(PACKAGE_BUGREPORT "https://github.com/bitcoin-ultimatum/orion/issues")

# currently the crossbuild is not supported
set(__IS_NATIVE_BUILD 1 CACHE INTERNAL "Indicate if this is a native build")

# set default cmake build type to RelWithDebInfo
# possible options are: None Debug Release RelWithDebInfo MinSizeRel
IF( NOT CMAKE_BUILD_TYPE )
    SET( CMAKE_BUILD_TYPE "RelWithDebInfo" )
ENDIF()

# Trying to set toolchain if not presented
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(CMAKE_TOOLCHAIN_FILE ./cmake/platforms/OSX.cmake)
        set(ENV{target} "Mac")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        set(CMAKE_TOOLCHAIN_FILE ./cmake/platforms/Linux64.cmake)
        set(ENV{target} "Linux")
    elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
          set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
        endif()
    endif()
endif()

set(CMAKE_USER_MAKE_RULES_OVERRIDE
	"${CMAKE_SOURCE_DIR}/cmake/scripts/OverrideInitFlags.cmake"
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG 1)
cmake_policy(SET CMP0077 NEW)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH "Archive output dir.")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" CACHE PATH "Library output dir.")
set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/bin" CACHE PATH "PDB (MSVC debug symbol)output dir.")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE PATH "Executable/dll output dir.")

add_custom_target(print-version
  COMMENT "Print the current ${PROJECT_NAME} version"
  COMMAND "${CMAKE_COMMAND}" -E echo "${PROJECT_VERSION}"
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if (CMAKE_GENERATOR MATCHES "Visual Studio" OR MSVC)
        message(WARNING "Important: Sometimes Visual Studio may get accidental crush due to low RAM memory. Please make sure you have enough free RAM for the build. Also is it recommended to turn off Real time protection of the Windows Defender in a build time.")
    endif()
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "Default triplet for the build" FORCE)
    set(VCPKG_ROOT "$ENV{VCPKG_ROOT}"  CACHE STRING "VCPKG root" FORCE)

    set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2" CACHE STRING "Enforced CXX static flag for release" FORCE)
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd  /Zi /Ob0 /Od /RTC1" CACHE STRING "Enforced CXX static flag for debug" FORCE)
    set(CMAKE_CXX_FLAGS_MINSIZEREL "/MT /O1 /Ob1" CACHE STRING "Enforced CXX static flag for minsizerel" FORCE)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /Zi /O2 /Ob1" CACHE STRING "Enforced CXX static flag for relwithdebinfo" FORCE)

    # /MD /O2 /Ob2
    set(CMAKE_C_FLAGS_RELEASE "/MT  /O2 /Ob2" CACHE STRING "Enforced C static flag for release" FORCE)

    # /MDd /Zi /Ob0 /Od /RTC1
    set(CMAKE_C_FLAGS_DEBUG "/MTd  /Zi /Ob0 /Od /RTC1" CACHE STRING "Enforced C static flag for debug" FORCE)

    # /MD /O1 /Ob1
    set(CMAKE_C_FLAGS_MINSIZEREL "/MT /O1 /Ob1" CACHE STRING "Enforced C static flag for minsizerel" FORCE)

    # /MD /Zi /O2 /Ob1
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "/MT /Zi /O2 /Ob1" CACHE STRING "Enforced C static flag for relwithdebinfo" FORCE)
endif()

# Add path for custom modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/scripts)

# Make contrib script accessible.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/contrib/cmake)

if(MSVC)
    # set additional key config values

    include(VCPKGHelper)
    config_msvc()
endif()

# TODO: implement all conditions:

# ENABLE_LEASING_MANAGER --disable-leasing-manager   --  ENABLE_LEASING_MANAGER, configure flag, no cmake
# ENABLE_WALLET --disable-wallet  --  ENABLE_WALLET, configure flag, no cmake
# ENABLE_TESTS --disable-tests 
# ENABLE_QT bitcoin_enable_qt --with-gui  --  ENABLE_GUI, configure flag, no cmake
# ENABLE_QT_TESTS --disable-gui-tests 
# ENABLE_BENCH --disable-bench 
# USE_QTCHARTS --with-qtcharts  --  ENABLE_QTCHARTS, configure flag, no cmake
# USE_LCOV --enable-lcov 
# GLIBC_BACK_COMPAT --enable-glibc-back-compat  --  ENABLE_GLIBC_BACK_COMPAT, configure flag, no cmake
# HARDEN --disable-hardening  --  DISABLE_HARDENING, configure flag, no cmake
# USE_ASM --enable-asm   --  ENABLE_ASM, configure flag, no cmake
# BUILD_BITCOIND --with-daemon 
# BUILD_BITCOIN_UTILS --with-utils  --  BUILD_UTILS, configure flag, no cmake
# BUILD_BITCOIN_LIBS --with-libs  --  BUILD_LIBS, configure flag, no cmake
# ENABLE_ZMQ --disable-zmq  --  ENABLE_ZMQ, configure flag, no cmake
# USE_UPNP --with-miniupnpc  --  WITH_MINIUNPC, configure flag, no cmake
# use_upnp_default --enable-upnp-default  --  START_WITH_UPNP, configure flag, no cmake
# EXTENDED_FUNCTIONAL_TESTS --enable-extended-functional-tests
# use_reduce_exports --enable-reduce-exports  --  ENABLE_REDUCE_EXPORTS, configure flag, no cmake
# use_ccache --disable-ccache
# use_lcov_branch --enable-lcov-branch-coverage
# system_univalue --with-system-univalue
# USE_NUM_GMP USE_NUM_OPENSSL --with-zerocoin-bignum
# PROTOC --with-protoc-bindir
# ENABLE_MAN --disable-man
# enable_debug --enable-debug
# SANITIZER_CXXFLAGS --with-sanitizers
# enable_gprof --enable-gprof  --  ENABLE_GPROF, configure flag, no cmake
# enable_werror --enable-werror
# USE_COVERAGE --coverage  --  ENABLE_COVERAGE, configure flag, no cmake

option(ENABLE_GUI "Build btcu-qt" ON)
option(ENABLE_QTCHARTS "Enable qtcharts support (default is yes if qt is enabled and qtchartview is found)" ON)
option(ENABLE_ASM "Enable assembly routines (default is yes)" ON)
option(ENABLE_COVERAGE "Enable coverage" OFF)
option(ENABLE_ZMQ "Activate the ZMQ notifications" ON)
option(ENABLE_WALLET "Activate the wallet functionality" ON)
option(BUILD_UTILS "Build btcu-cli and btcu-tx" ON)
option(BUILD_LIBS "Build the btcuconsensus shared library" ON)
option(DISABLE_HARDENING "Do not attempt to harden the resulting executables" OFF)
option(ENABLE_REDUCE_EXPORTS "Attempt to reduce exported symbols in the resulting" OFF)
option(ENABLE_GLIBC_BACK_COMPAT "Enable backwards compatibility with glibc" OFF)
option(WITH_MINIUNPC "Enable UPnP support" ON)
option(START_WITH_UPNP "If UPNP is enabled, turn it on at startup" OFF)
option(ENABLE_GPROF "Use gprof profiling compiler flags " OFF)
option(ENABLE_LEASING_MANAGER "Enable leasing manager" ON)
option(ENABLE_TESTS "Build tests" OFF)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(DEFAULT_ENABLE_DBUS_NOTIFICATIONS ON)
endif()
option(ENABLE_DBUS_NOTIFICATIONS "Enable DBus desktop notifications. Linux only." ${DEFAULT_ENABLE_DBUS_NOTIFICATIONS})

# collect config flags
set(PROJECT_CONFIGURE_FLAGS "")
if(ENABLE_GUI)
    message(STATUS "Enabled option: Build btcu-qt")
else()
    message(STATUS "Disabled option: Build btcu-qt")
endif()
if(ENABLE_LEASING_MANAGER)
    message(STATUS "Enabled option: Leasing manager")
else()
    message(STATUS "Disabled option: Leasing manager")
endif()
if(ENABLE_ZMQ)
    message(STATUS "Enabled option: Activate the ZMQ notifications")
else()
    message(STATUS "Disabled option: Activate the ZMQ notifications")
endif()
if(ENABLE_WALLET)
    message(STATUS "Enabled option: Activate the wallet functionality")
else()
    message(STATUS "Disabled option: Activate the wallet functionality")
endif()
if(BUILD_UTILS)
    message(STATUS "Enabled option: Build btcu-cli and btcu-tx")
else()
    message(STATUS "Disabled option: Build btcu-cli and btcu-tx")
endif()
if(BUILD_LIBS)
    message(STATUS "Enabled option: Build the btcuconsensus shared library")
else()
    message(STATUS "Disabled option: Build the btcuconsensus shared library")
endif()
if(DISABLE_HARDENING)
    message(STATUS "Disabled option: Harden the executables")
else()
    message(STATUS "Enabled option: Harden the executables")
endif()
if(ENABLE_REDUCE_EXPORTS)
    message(STATUS "Enabled option: Attempt to reduce exported symbols in the resulting")
else()
    message(STATUS "Disabled option: Attempt to reduce exported symbols in the resulting")
endif()
if(ENABLE_GLIBC_BACK_COMPAT)
    message(STATUS "Enabled option: Backwards compatibility with glibc")
else()
    message(STATUS "Disabled option: Backwards compatibility with glibc")
endif()
if(WITH_MINIUNPC)
    message(STATUS "Enabled option: UPnP support")
else()
    message(STATUS "Disabled option: UPnP support")
endif()
if(START_WITH_UPNP)
    message(STATUS "Enabled option: If UPNP is enabled, turn it on at startup")
else()
    message(STATUS "Disabled option: If UPNP is enabled, turn it on at startup")
endif()
if(ENABLE_GPROF)
    message(STATUS "Enabled option: Use gprof profiling compiler flags")
else()
    message(STATUS "Disabled option: Use gprof profiling compiler flags")
endif()
if(ENABLE_TESTS)
    message(STATUS "Enabled option: Build tests")
else()
    message(STATUS "Disabled option: Build tests")
endif()

# If ccache is available, then use it.
find_program(CCACHE ccache)
if(CCACHE)
	message(STATUS "Using ccache: ${CCACHE}")
	set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
	set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif(CCACHE)

# flags
include(AddCompilerFlags)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# Ensure that WINDRES_PREPROC is enabled when using windres.
	list(APPEND CMAKE_RC_FLAGS "-DWINDRES_PREPROC")
    
    if(NOT MSVC)
        add_compiler_flags(
            -Wstack-protector 
            -fstack-protector-all 
            -fPIE 
            -pipe 
            -O2 
            -pthread 
            -Wl
            --dynamicbase
            -nxcompat
            --high-entropy-va
            -pie
        )

        # Build all static so there is no dll file to distribute.
        add_linker_flags(-static)
    endif()
else()
    # Prefer -g3, defaults to -g if unavailable
    foreach(LANGUAGE C CXX)
        set(COMPILER_DEBUG_LEVEL -g)
        check_compiler_flags(G3_IS_SUPPORTED ${LANGUAGE} -g3)
        if(${G3_IS_SUPPORTED})
            set(COMPILER_DEBUG_LEVEL -g3)
        endif()
        add_compile_options_to_configuration_for_language(Debug ${LANGUAGE} ${COMPILER_DEBUG_LEVEL})
    endforeach()

    # Add -ftrapv when building in Debug
    add_compile_options_to_configuration(Debug -ftrapv)

    # All versions of gcc that we commonly use for building are subject to bug
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90348. To work around that, set
    # -fstack-reuse=none for all gcc builds. (Only gcc understands this flag)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compiler_flags(-fstack-reuse=none)
    endif()

    # enforce fPIC and no-stack-check to avoid some SEGFAULT cases with shared libs
    add_cxx_compiler_flags(-fPIC -fno-stack-check)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_compile_definitions(MAC_OSX OBJC_OLD_DISPATCH_PROTOTYPES=0)
	add_linker_flags(-Wl,-dead_strip_dylibs)
endif()

if(ENABLE_REDUCE_EXPORTS)
	# Default visibility is set by CMAKE_<LANG>_VISIBILITY_PRESET, but this
	# doesn't tell if the visibility set is effective.
	# Check if the flag -fvisibility=hidden is supported, as using the hidden
	# visibility is a requirement to reduce exports.
	check_compiler_flags(HAS_CXX_FVISIBILITY CXX -fvisibility=hidden)
	if(NOT HAS_CXX_FVISIBILITY)
		message(FATAL_ERROR "Cannot set default symbol visibility. Use -DENABLE_REDUCE_EXPORTS=OFF.")
	endif()

	# Also hide symbols from static libraries
	add_linker_flags(-Wl,--exclude-libs,libstdc++)
endif()


if(ENABLE_HARDENING AND NOT MSVC)
	# Enable stack protection
	add_cxx_compiler_flags(-fstack-protector-all -Wstack-protector)

	# Enable some buffer overflow checking, except in -O0 builds which
	# do not support them
	add_compiler_flags(-U_FORTIFY_SOURCE)
	add_compile_options($<$<NOT:$<CONFIG:Debug>>:-D_FORTIFY_SOURCE=2>)

	# Enable ASLR (these flags are primarily targeting MinGw)
	add_linker_flags(-Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va)

	# Make the relocated sections read-only
	add_linker_flags(-Wl,-z,relro -Wl,-z,now)

	# CMake provides the POSITION_INDEPENDENT_CODE property to set PIC/PIE.
	cmake_policy(SET CMP0083 NEW)
	include(CheckPIESupported)
	check_pie_supported()

	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		# MinGw provides its own libssp for stack smashing protection
		link_libraries(ssp)
	endif()
endif()

if (NOT MSVC)
    # Enable warning
    add_c_compiler_flags(-Wnested-externs -Wstrict-prototypes)
    add_compiler_flags(
        -Wall
        -Wextra
        -Wformat
        -Wvla
        -Wcast-align
        -Wunused-parameter
        -Wmissing-braces
        -Wthread-safety
        -Wshadow
        -Wshadow-field
        -Wrange-loop-analysis
        -Wredundant-decls
        -Wunreachable-code-loop-increment
    )
    add_compiler_flag_group(-Wformat -Wformat-security)
    add_cxx_compiler_flags(
        -Wredundant-move
    )

    option(EXTRA_WARNINGS "Enable extra warnings" OFF)
    if(EXTRA_WARNINGS)
        add_cxx_compiler_flags(-Wsuggest-override)
    else()
        add_compiler_flags(-Wno-unused-parameter)
        add_compiler_flags(-Wno-implicit-fallthrough)
    endif()
endif()

if (MSVC)
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_STATIC_LIBS ON)

    add_compile_options(/bigobj)                    # disable C1128	number of sections exceeded object file format limit
    add_compile_options(/MP)                        # enable parallel compilation
    add_compile_options(/EHsc)                      # specify Exception Handling Model in msvc
    add_compile_options(/wd4068)                    # disable unknown pragma warning (4068)
    add_compile_options(/wd4100)                    # disable unreferenced formal parameter (4100)
    add_compile_options(/wd4514)                    # unreferenced inline function has been removed (4514)
    add_compile_options(/wd4996)                    # disable unsafe function warning (4996)
    add_compile_options(/wd4503)                    # disable decorated name length exceeded, name was truncated (4503)
    add_compile_options(/wd4267)                    # disable conversion from 'size_t' to 'type', possible loss of data (4267)
    add_compile_options(/wd4180)                    # disable qualifier applied to function type has no meaning; ignored (4180)
    add_compile_options(/wd4290)                    # disable C++ exception specification ignored except to indicate a function is not __declspec(nothrow) (4290)
    add_compile_options(/wd4297)                    # disable <vector>'s function assumed not to throw an exception but does (4297)
    add_compile_options(/wd4242)                    # disable conversion from 'type1' to 'type2', possible loss of data (4242)
    add_compile_options(/wd4244)                    # disable conversion from 'type1' to 'type2', possible loss of data (4244)
    add_compile_options(/wd4800)                    # disable forcing value to bool 'true' or 'false' (performance warning) (4800)
    add_compile_options(/wd4702)                    # disable unreachable code (4702)
    add_compile_options(/wd4820)                    # disable warning % bytes padding added after data member % (4820)
    add_compile_options(/wd4668)                    # disable warning % is not defined as a preprocessor macro, replacing with '0' for % (4668)
    add_compile_options(/wd5045)                    # disable warning Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified (5045)
    add_compile_options(-DWINVER=0x0A00)            # declare Windows API requirement
    add_compile_options(-D_WIN32_WINNT=0x0A00)      # declare Windows API requirement
    add_compile_options(-DNTDDI_VERSION=0x0A000001) # declare Windows API requirement
    add_compile_options(-DNOMINMAX)                 # undefine windows.h MAX && MIN macros cause it cause conflicts with std::min && std::max functions
    add_compile_options(-DMINIUPNP_STATICLIB)       # define miniupnp static library

	add_compile_definitions(
		# Define WIN32_LEAN_AND_MEAN to exclude APIs such as Cryptography, DDE,
		# RPC, Shell, and Windows Sockets.
		WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0A00
        WINVER=0x0A00
        NTDDI_VERSION=0x0A000001
        /Zc:wchar_t # Treat wchar_t as built-in type
	)

    # disable empty object file warning
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221 /NODEFAULTLIB:MSVCRT -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00")
    # warning LNK4075: ignoring '/EDITANDCONTINUE' due to '/SAFESEH' specification
    # warning LNK4099: pdb was not found with lib
    # stack size 16MB
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099,4075 /STACK:16777216 /NODEFAULTLIB:MSVCRT -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00")
else()
    set(BUILD_SHARED_LIBS ON)
    set(BUILD_STATIC_LIBS ON)
endif ()

# Setting up Boost parameters and paths to dependencies
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/boost)
    list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/curl)
    set( Boost_INCLUDE_DIR "/usr/local/opt/boost/include" )
    set( BOOST_LIBRARYDIR "/usr/local/opt/boost/lib" )
    set( BOOST_ROOT "/usr/local/opt/boost" )
    set( Boost_NO_BOOST_CMAKE ON )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_package(Threads)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/db-18.1.32/LICENSE")
        set( BerkeleyDB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/db-18.1.32/build_windows" )

        if(${CMAKE_BUILD_TYPE} MATCHES "DEBUG" OR ${CMAKE_BUILD_TYPE} MATCHES "Debug")
            set( BerkeleyDB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/db-18.1.32/build_windows/x64/Static Debug/" )
        else()
            set( BerkeleyDB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/db-18.1.32/build_windows/x64/Static Release/" )
        endif()
    else()
        message(FATAL_ERROR "The required Berkeley DB package is not installed. Please check installation instruction boost section!")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/boost_1_71_0/INSTALL")
    
        option(CMAKE_USE_WIN32_THREADS_INIT "using WIN32 threads" ON)
        add_definitions(-DBOOST_USE_WINAPI_VERSION=0x0A00)
        add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
        
        # add_definitions(-DBOOST_SYSTEM_USE_UTF8 )
      
        set( BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/boost_1_71_0" ) 

        set( BOOST_INCLUDEDIR "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/boost_1_71_0/boost" )
        set( Boost_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/boost_1_71_0" )

        set( BOOST_LIBRARYDIR "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/boost_1_71_0/stage/lib" )

        set( Boost_LIBRARY_DIR_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/boost_1_71_0/stage/lib" )
        set( Boost_LIBRARY_DIR_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/depends/packages/static/boost_1_71_0/stage/lib" )
        
	    set(Boost_THREADAPI win32)

        set( Boost_NO_SYSTEM_PATHS ON )
        set( Boost_USE_STATIC_LIBS ON )
        set( Boost_USE_STATIC_RUNTIME ON )

    else()
        message(FATAL_ERROR "The required Boost package is not installed. Please check installation instruction boost section!")
    endif()
else()
	list( APPEND CMAKE_PREFIX_PATH /usr/lib )
	list( APPEND CMAKE_PREFIX_PATH /usr/lib/x86_64-linux-gnu )
    
    set( BOOST_ROOT "usr" )
    set( Boost_INCLUDE_DIR "/usr/include" )
    set( Boost_NO_BOOST_CMAKE ON )
    set( Boost_USE_STATIC_LIBS  OFF )
    set( Boost_USE_STATIC_RUNTIME OFF )
endif()

set( Boost_USE_MULTITHREADED ON )
set( Boost_NO_SYSTEM_PATHS ON )

if(${CMAKE_BUILD_TYPE} MATCHES "DEBUG" OR ${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set( Boost_USE_DEBUG_RUNTIME ON )
    set( Boost_USE_RELEASE_LIBS OFF )
    set( Boost_USE_DEBUG_LIBS ON )
else()
    set( Boost_USE_DEBUG_RUNTIME OFF )
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DBOOST_ALL_STATIC_LINK=1)
    add_definitions(-DBOOST_LIB_DIAGNOSTIC=1)
    add_definitions(-DBOOST_LOG_BUILDING_THE_LIB=1)
    add_definitions(-DBOOST_THREAD_USE_LIB=1)

    add_compile_definitions(
        BOOST_ALL_STATIC_LINK=1
        BOOST_ALL_NO_LIB=1
        BOOST_LOG_BUILDING_THE_LIB=1
        BOOST_THREAD_USE_LIB=1
	)
else()
    add_definitions(-DBOOST_LOG_DYN_LINK=1)
    add_definitions(-DBOOST_ALL_DYN_LINK=1)
endif()

add_definitions(-DBOOST_NO_SCOPED_ENUMS=1)
add_definitions(-DBOOST_NO_CXX11_SCOPED_ENUMS=1)

find_package(Boost 1.71.0 EXACT COMPONENTS program_options filesystem system thread context fiber chrono log_setup log unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

add_definitions(-DETH_FATDB=1)

message(STATUS Boost_LIBRARIES:)
message (STATUS ${Boost_LIBRARIES})

message(STATUS Boost_INCLUDE_DIRS:)
message (STATUS ${Boost_INCLUDE_DIRS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    link_directories(${BOOST_LIBRARYDIR})
endif()

include(BrewHelper)

list(APPEND CMAKE_PREFIX_PATH ./src/jsoncpp)
list(APPEND CMAKE_PREFIX_PATH ./src/libscrypt)
list(APPEND CMAKE_PREFIX_PATH ./src/cryptopp)
list(APPEND CMAKE_PREFIX_PATH ./src/cpp-ethereum/ethash)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/qt5)
	list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/berkeley-db)
	list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/rocksdb)
	list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/libjson-rpc-cpp)
	list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/google-benchmark)
	list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/snappy)
	list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/libevent)
    
	set(BerkeleyDB_ROOT_DIR "/usr/local/opt/berkeley-db")

	set( CURL_INCLUDE_DIR "/usr/local/opt/curl/include" )
	set( CURL_LIBRARY "/usr/local/opt/curl/lib" )

	set( MHD_INCLUDE_DIR "/usr/local/opt/libmicrohttpd/include" )
	set( MHD_LIBRARY "/usr/local/opt/libmicrohttpd/lib" )

	set( BerkeleyDB_INCLUDE_DIR "/usr/local/opt/berkeley-db/include" )
	set( BerkeleyDB_LIBRARY "/usr/local/opt/berkeley-db/lib" )

	find_brew_prefix(OPENSSL_ROOT_DIR openssl)
	find_brew_prefix(ZMQ_ROOT_DIR zmq)

	set( LibEvent_INCLUDE_DIR "/usr/local/opt/libevent/include" )
	set( LibEvent_LIBRARY "/usr/local/opt/libevent/lib" )

	set( GMP_INCLUDE_DIR "/usr/local/opt/gmp/include" )
	set( GMP_LIBRARY "/usr/local/opt/gmp/lib" )

	set( ZeroMQ_INCLUDE_DIR "${ZMQ_ROOT_DIR}/include" )
	set( ZeroMQ_LIBRARY "${ZMQ_ROOT_DIR}/lib" )

	set( Qrcode_INCLUDE_DIR "/usr/local/opt/qrencode/include" )
	set( Qrcode_LIBRARY "/usr/local/opt/qrencode/lib" )
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set( GTest_DIR "/usr/src/googletest" CACHE PATH "Path to googletest")
	set( GTEST_ROOT "/usr/src/googletest" CACHE PATH "Path to googletest")
	set( GMP_INCLUDE_DIR "/usr/include/x86_64-linux-gnu" CACHE STRING "")
	set( RocksDB_INCLUDE_DIR "/usr/include" )
	set( ZeroMQ_INCLUDE_DIR "/usr/include" CACHE STRING "")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/Qt5Core")
	list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/rocksdb")
	list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/zeromq")
	list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/snappy")
	list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/curl")
	list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/protobuf")
	list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/qt5/bin")

    if(${CMAKE_BUILD_TYPE} MATCHES "DEBUG" OR ${CMAKE_BUILD_TYPE} MATCHES "Debug")
	    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib")
    else()
	    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib")
    endif()
    
	set( CURL_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/curl"  CACHE STRING "Curl root dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/curl")

	set( OPENSSL_ROOT_DIR "${VCPKG_ROOT}/packages/openssl_${VCPKG_TARGET_TRIPLET}" CACHE STRING "OpenSSL root dir is ${VCPKG_ROOT}/packages/openssl_${VCPKG_TARGET_TRIPLET}")
    
	set( Qt5Core_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Core" CACHE STRING "QT5Core root dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Core")
    set( Qt5Widgets_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Widgets" CACHE STRING "Qt5Widgets root dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Widgets")
    set( Qt5Gui_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Gui" CACHE STRING "Qt5Gui root dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Gui")
    set( Qt5Network_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Network" CACHE STRING "Qt5Network root dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Network")
    set( Qt5LinguistTools_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5LinguistTools" CACHE STRING "Qt5LinguistTools root dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5LinguistTools")
    set( Qt5DBus_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5DBus" CACHE STRING "Qt5DBus root dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5DBus")
    set( Qt5Svg_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Svg" CACHE STRING "Qt5Svg root dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Svg")
    set( Qt5Charts_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Charts" CACHE STRING "QT5Core root dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Charts")
    set( Qt5Test_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Test" CACHE STRING "QT5Core root dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/cmake/Qt5Test")

	set( MOC_BIN_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/qt5/bin")

    if(${CMAKE_BUILD_TYPE} MATCHES "DEBUG" OR ${CMAKE_BUILD_TYPE} MATCHES "Debug")
	    set( ROCKSDB_ROOT_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/debug" )
	    set( MHD_LIBRARY "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib" )
	    set( GMP_LIBRARY "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib" )
    else()
	    set( ROCKSDB_ROOT_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}" )
	    set( MHD_LIBRARY "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib" )
	    set( GMP_LIBRARY "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib" )
    endif()

	set( CURL_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include" )

	set( MHD_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include" )
    
	set( LibEvent_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include" )

	set( GMP_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include" CACHE STRING "GMP_INCLUDE_DIR include dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")

	set( ZeroMQ_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include" )

	set( Qrcode_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include" )

	set( Protobuf_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include" )

	set( MINIUPNP_INCLUDE_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include/miniupnpc" CACHE STRING "MINIUPNP_INCLUDE_DIR include dir is ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")

    include_directories("${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")

    # To access the config/ directory
    include_directories("${CMAKE_CURRENT_BINARY_DIR}/src")
	list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}")
	list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/bin")
	list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/lib")

    # set prefixes globally 
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "Global prefix path" FORCE)
    
	find_library(COMCTL32_LIBRARY NAMES comctl32)
	find_library(COMDLG32_LIBRARY NAMES comdlg32)
	find_library(CRYPT32_LIBRARY NAMES crypt32)
	find_library(GDI32_LIBRARY NAMES gdi32)
    find_library(IMM32_LIBRARY NAMES imm32)
    find_library(IPHLPAPI_LIBRARY NAMES iphlpapi)
    find_library(KERNEL32_LIBRARY NAMES kernel32)
    find_library(MSWSOCK_LIBRARY NAMES mswsock)
    find_library(OLE32_LIBRARY NAMES ole32)
    find_library(OLEAUT32_LIBRARY NAMES oleaut32)
    find_library(RPCRT4_LIBRARY NAMES rpcrt4)
    find_library(RT_LIBRARY NAMES rt)
    find_library(SHELL32_LIBRARY NAMES shell32)
    find_library(SHLWAPI_LIBRARY NAMES shlwapi)
    find_library(SSP_LIBRARY NAMES ssp)
    find_library(USER32_LIBRARY NAMES user32)
    find_library(UUID_LIBRARY NAMES uuid)
    find_library(WINMM_LIBRARY NAMES winmm)
    find_library(WINSPOOL_LIBRARY NAMES winspool)
    find_library(WS2_32_LIBRARY NAMES ws2_32)
    find_library(Z_LIBRARY NAMES zlib z)

    include(WindowsVersionInfo)
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    message(STATUS "Found OpenSSL (${OPENSSL_VERSION}): ${OPENSSL_LIBRARIES}")

    message(STATUS OPENSSL_INCLUDE_DIR:)
    message (STATUS ${OPENSSL_INCLUDE_DIR})

    if(OPENSSL_VERSION VERSION_GREATER_EQUAL 1.1)
        message(STATUS "Found unsupported OpenSSL version!")
    endif()
endif()

find_package(GMP)
if(GMP_FOUND)
    message(STATUS "Found GMP (${GMP_VERSION}): ${GMP_LIBRARIES}")

    message(STATUS GMP_INCLUDE_DIR:)
    message (STATUS ${GMP_INCLUDE_DIRS})
else()
    message(WARNING "GMP not found, falling back to OpenSSL for bignum!")
endif()

find_package(miniupnp)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_package(miniupnpc)
endif()
if(MINIUPNP_FOUND)
    message(STATUS "Found miniupnp (${MINIUPNP_VERSION}): ${MINIUPNP_LIBRARIES}")
else()
    message(WARNING "Miniupnp not found!")
endif()
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        find_package(libjson-rpc-cpp CONFIG REQUIRED)
    else()
	    find_package(libjson-rpc-cpp REQUIRED)
    endif()
endif()

set(BDB_VER "18.1.32")

# libtool style configure
add_subdirectory(src/config)
add_definitions(-DHAVE_CONFIG_H)

# Enable LFS (Large File Support) on targets that don't have it natively.
# This should be defined before the libraries are included as leveldb need the
# definition to be set.
if(NOT HAVE_LARGE_FILE_SUPPORT)
	add_compile_definitions(_FILE_OFFSET_BITS=64)
	add_linker_flags(-Wl,--large-address-aware)
endif()

if(ENABLE_GLIBC_BACK_COMPAT)
	# Wrap some glibc functions with ours
	add_linker_flags(-Wl,--wrap=__divmoddi4)
	add_linker_flags(-Wl,--wrap=log2f)

	if(NOT HAVE_LARGE_FILE_SUPPORT)
		add_linker_flags(-Wl,--wrap=fcntl -Wl,--wrap=fcntl64)
	endif()
endif()
    
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    file(READ "/proc/version" _SYS_VERSION)
    string(REGEX MATCH "Microsoft" _SYSTEM_VERSION_PARSED "${_SYS_VERSION}")
    if(${_SYSTEM_VERSION_PARSED} MATCHES "Microsoft")
        message(STATUS "Compiling on WSL")
        set(ENV{DISPLAY} ":0")
        set(ENV{LIBGL_ALWAYS_INDIRECT} 1)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY /home/$ENV{USER}/btcu-run)
        message(WARNING "WSL Runtime support requires VcXsrv to be installed and running")
    endif()
endif()
 
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_package(PkgConfig REQUIRED) 
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_package(ZeroMQ CONFIG)
else()
    find_package(ZeroMQ)
endif()
if(ZeroMQ_FOUND)
    set( ZeroMQ_LIBRARY "${ZeroMQ_zmq_LIBRARY}" CACHE STRING "")
    message(STATUS "Found ZeroMQ (${ZeroMQ_VERSION}): ${ZeroMQ_LIBRARY}")

    message(STATUS ZeroMQ_INCLUDE_DIRS:)
    message (STATUS ${ZeroMQ_INCLUDE_DIRS})
    
    message(STATUS ZeroMQ_LIBRARY:)
    message (STATUS ${ZeroMQ_LIBRARY})
    
    message(STATUS ZeroMQ_STATIC_LIBRARY:)
    message (STATUS ${ZeroMQ_STATIC_LIBRARY})
else()
    message(WARNING "ZeroMQ not found!")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_package(RocksDB CONFIG REQUIRED)
else()
    find_package(RocksDB REQUIRED)
endif()
if(RocksDB_FOUND)
    message(STATUS "Found RocksDB")
    message(STATUS RocksDB_LIBRARIES:)
    message (STATUS ${RocksDB_LIBRARIES})
    message(STATUS ROCKSDB_LIBRARY:)
    message (STATUS ${ROCKSDB_LIBRARY})
    message(STATUS RocksDB_INCLUDE_DIR:)
    message (STATUS ${RocksDB_INCLUDE_DIR})
    message(STATUS RocksDB_INCLUDE_DIRS:)
    message (STATUS ${RocksDB_INCLUDE_DIRS})
else()
    message(WARNING "RocksDB not found! BTCU requires this functionality")
endif()

find_package(Snappy CONFIG REQUIRED)
if(Snappy_FOUND)
    message(STATUS "Found Snappy")
    message(STATUS Snappy_LIBRARIES:)
    message (STATUS ${Snappy_LIBRARIES})
    message(STATUS Snappy_INCLUDE_DIRS:)
    message (STATUS ${Snappy_INCLUDE_DIRS})
else()
    message(WARNING "Snappy not found! BTCU requires this functionality")
endif()

# Find Dependencies
find_package(BerkeleyDB REQUIRED)
if(BerkeleyDB_FOUND)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set( BerkeleyDB_Cxx_LIBRARY "${BerkeleyDB_LIBRARY}" )
    endif()
    message(STATUS "Found BerkeleyDB (${BerkeleyDB_VERSION}): ${BerkeleyDB_Cxx_LIBRARY}")
    
    message(STATUS BerkeleyDB_INCLUDE_DIRS:)
    message (STATUS ${BerkeleyDB_INCLUDE_DIRS})
    
    get_filename_component(BerkeleyDB_Cxx_LIBRARY_DIRS "${BerkeleyDB_Cxx_LIBRARY}" DIRECTORY)
    message(STATUS BerkeleyDB_LIBRARY_DIRS:)
    message (STATUS ${BerkeleyDB_Cxx_LIBRARY_DIRS})
    
    if(${BerkeleyDB_VERSION} MATCHES "..")
        message(WARNING "Unknown version Berkeley DB. If this is not ${BDB_VER}, please reinstall it!")
    else()
        if(NOT ${BerkeleyDB_VERSION} MATCHES "${BDB_VER}")
            message(WARNING "BerkeleyDB version other than ${BDB_VER} found: ${BerkeleyDB_VERSION}!")
        endif()
    endif()
else()
    message(WARNING "BerkeleyDB not found! BTCU Core requires this library for wallet functionality (-DENABLE_WALLET=OFF to disable wallet functionality)")
endif()

find_package(LibEvent REQUIRED)

# Find the python interpreter. This is required for several targets.
find_package(Python 3.5 COMPONENTS Interpreter REQUIRED)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    include(${CMAKE_SOURCE_DIR}/contrib/cmake/jsonrpc.cmake)
endif()

add_subdirectory(src/secp256k1)
add_subdirectory(src/univalue)
add_subdirectory(src/libff)
add_subdirectory(src/libscrypt)
add_subdirectory(src/cryptopp)
add_subdirectory(src/jsoncpp/jsoncpp)

# dependencies
set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

add_subdirectory(src/cpp-ethereum/ethash)

# remove all NDEBUG flags due to it is restricted option
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
    string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
    string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
endif()

link_directories(${CMAKE_BINARY_DIR})

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
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(libleveldb_cpp_flags -DOS_WINDOWS -DLEVELDB_PLATFORM_WINDOWS)
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

set(LEVELDB_COMPILE_OPTIONS -Wno-implicit-fallthrough -Wno-inconsistent-missing-override -Wno-maybe-uninitialized -Wno-shadow -Wno-sign-compare -Wno-unused-const-variable)

add_library(leveldb ${libleveldb_a_headers} ${libleveldb_a_sources})
if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_compile_options(leveldb PRIVATE ${LEVELDB_COMPILE_OPTIONS})
endif()
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

target_link_libraries(leveldb leveldb_sse42)

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
        
        # currently used only as experimental feature on Windows
        ./src/shutdown.cpp
        )


add_library(SERVER_A STATIC ${BitcoinHeaders} ${SERVER_SOURCES})
if(MINIUPNP_FOUND)
    target_include_directories(SERVER_A PUBLIC
        ${MINIUPNP_INCLUDE_DIR}
        )
    target_link_libraries(SERVER_A 
        ${MINIUPNP_LIBRARY}
        )
endif()
target_include_directories(SERVER_A
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers/memenv
        ${ZeroMQ_INCLUDE_DIRS}
        ${LibEvent_INCLUDE_DIR}
        ${BerkeleyDB_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}/filesystem
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
add_dependencies(SERVER_A leveldb)
target_link_libraries(SERVER_A 
        univalue
        UTIL_A
        BITCOIN_CRYPTO_A
        COMMON_A
        ${Boost_LIBRARIES}
        ${LibEvent_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        leveldb
        secp256k1
        RocksDB::rocksdb
        ${CMAKE_DL_LIBS}
        Threads::Threads 
        ff 
        Snappy::snappy
        Threads::Threads
        scrypt
        )
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(SERVER_A 
        ${ZeroMQ_STATIC_LIBRARY} jsoncpp_lib_static cryptopp-static Oracle::BerkeleyDB
        )
    if(CRYPT32_LIBRARY)
        target_link_libraries(SERVER_A 
            ${CRYPT32_LIBRARY}
        )
    endif()
else()
    target_link_libraries(SERVER_A 
        ${ZeroMQ_LIBRARY} jsoncpp_lib cryptopp-shared ${BerkeleyDB_Cxx_LIBRARY}
        )
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

if(ZeroMQ_FOUND)
    set(ZMQ_SOURCES
        ./src/zmq/zmqabstractnotifier.cpp
        ./src/zmq/zmqnotificationinterface.cpp
        ./src/zmq/zmqpublishnotifier.cpp
    )
    add_library(ZMQ_A STATIC ${BitcoinHeaders} ${ZMQ_SOURCES})
    target_include_directories(ZMQ_A PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src ${ZeroMQ_INCLUDE_DIRS} 
        ${OPENSSL_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        )
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        target_link_libraries(ZMQ_A 
            ${ZeroMQ_STATIC_LIBRARY}
            )
    else()
        target_link_libraries(ZMQ_A 
            ${ZeroMQ_LIBRARY}
            )
    endif()
    if(GMP_FOUND)
        target_include_directories(ZMQ_A PUBLIC ${GMP_INCLUDE_DIR})
    endif()
endif()

add_library(WALLET_A STATIC ${BitcoinHeaders} ${WALLET_SOURCES})
target_include_directories(WALLET_A PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/jsoncpp/jsoncpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/ethash/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/evmc/include
        ${OPENSSL_INCLUDE_DIR}
        ${BerkeleyDB_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        )
if(GMP_FOUND)
    target_include_directories(WALLET_A PUBLIC ${GMP_INCLUDE_DIR})
endif()
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
add_library(BITCOIN_CRYPTO_A ${BITCOIN_CRYPTO_SOURCES})
target_link_libraries(BITCOIN_CRYPTO_A
            univalue
            ${Boost_LIBRARIES} 
            ${OPENSSL_LIBRARIES}
            ${LibEvent_LIBRARIES}
            Threads::Threads
        )
target_include_directories(BITCOIN_CRYPTO_A PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENSSL_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
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
target_include_directories(COMMON_A 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/jsoncpp/jsoncpp/include
        ${OPENSSL_INCLUDE_DIR}
        ${BerkeleyDB_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/ethash/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/evmc/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/qtum
        )
target_link_libraries(COMMON_A
            BITCOIN_CRYPTO_A
            ${Boost_LIBRARIES} 
            ${OPENSSL_LIBRARIES}
            ${LibEvent_LIBRARIES}
            UTIL_A
            SERVER_A
        )
if (MSVC)
    target_link_libraries(COMMON_A jsoncpp_lib_static)
else()
    target_link_libraries(COMMON_A jsoncpp_lib)
endif()
if(GMP_FOUND)
    target_link_libraries(COMMON_A ${GMP_LIBRARIES})
    target_include_directories(COMMON_A PUBLIC ${GMP_INCLUDE_DIR})
endif()
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

add_library(ZEROCOIN_A ${ZEROCOIN_SOURCES})
target_include_directories(ZEROCOIN_A PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${OPENSSL_INCLUDE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/secp256k1/include
        )
target_link_libraries(ZEROCOIN_A
            UTIL_A
            BITCOIN_CRYPTO_A
            ${Boost_LIBRARIES} 
            ${OPENSSL_LIBRARIES}
            ${LibEvent_LIBRARIES}
            COMMON_A
            secp256k1
        )
if (MSVC)
    target_link_libraries(ZEROCOIN_A jsoncpp_lib_static)
else()
    target_link_libraries(ZEROCOIN_A jsoncpp_lib)
endif()
if(GMP_FOUND)
    target_link_libraries(ZEROCOIN_A ${GMP_LIBRARIES})
    target_include_directories(ZEROCOIN_A PUBLIC ${GMP_INCLUDE_DIR})
endif()

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
add_library(UTIL_A 
            ${BitcoinHeaders} 
            ${UTIL_SOURCES}
        )
        
target_include_directories(UTIL_A PUBLIC 
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
            ${OPENSSL_INCLUDE_DIR}
            ${Boost_LIBRARIES} 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/
        )
target_link_libraries(UTIL_A
            univalue
            BITCOIN_CRYPTO_A
            ${Boost_LIBRARIES} 
            ${OPENSSL_LIBRARIES}
            ${LibEvent_LIBRARIES}
            Threads::Threads
        )

set(CLI_A_SOURCES ./src/rpc/client.cpp)
add_library(CLI_A ${BitcoinHeaders} ${CLI_A_SOURCES})
target_include_directories(CLI_A 
            PUBLIC 
                ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include
        )
target_link_libraries(CLI_A
            univalue
            BITCOIN_CRYPTO_A
            ${Boost_LIBRARIES} 
            ${OPENSSL_LIBRARIES}
            ${LibEvent_LIBRARIES}
            Threads::Threads
        )

set(btcu-cli_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcu-cli.cpp)
add_executable(btcu-cli ${btcu-cli_SOURCES})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    generate_windows_version_info(btcu-cli
        DESCRIPTION "CLI BTCU transaction editor utility"
    )
    list(APPEND btcu-cli_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcu-cli-res.rc)
    if(CRYPT32_LIBRARY)
        target_link_libraries(btcu-cli
            ${CRYPT32_LIBRARY}
        )
    endif()
endif()
target_link_libraries(btcu-cli
        CLI_A
        univalue
        UTIL_A
        BITCOIN_CRYPTO_A
        ${Boost_LIBRARIES} 
        ${OPENSSL_LIBRARIES}
        ${LibEvent_LIBRARIES}
        Threads::Threads
        )
target_include_directories(btcu-cli 
            PUBLIC 
                ${LibEvent_INCLUDE_DIR}
        )

set(btcu-tx_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcu-tx.cpp)
add_executable(btcu-tx ${btcu-tx_SOURCES})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	generate_windows_version_info(btcu-tx
		DESCRIPTION "JSON-RPC client for ${PACKAGE_NAME}"
	)
    list(APPEND btcu-tx_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcu-tx-res.rc)
endif()
target_link_libraries(btcu-tx
        univalue
        COMMON_A
        SERVER_A
        ZEROCOIN_A
        UTIL_A
        BITCOIN_CRYPTO_A
        secp256k1
        ${Boost_LIBRARIES} 
        ${OPENSSL_LIBRARIES}
        ${LibEvent_LIBRARIES}
        )
if(GMP_FOUND)
    target_link_libraries(btcu-tx ${GMP_LIBRARIES})
endif()

set(btcud_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcud.cpp ${Boost_INCLUDE_DIRS})
add_executable(btcud ${btcud_SOURCES} ${BitcoinHeaders})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	generate_windows_version_info(btcud
		DESCRIPTION "BTCU node with a JSON-RPC server"
	)
    list(APPEND btcud_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/btcud-res.rc)
endif()
add_dependencies(btcud leveldb leveldb_sse42 memenv)
target_include_directories(btcud PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/leveldb/helpers/memenv
        ${btcud_SOURCES}
        ${LibEvent_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS})
target_link_libraries(btcud
        Threads::Threads
        SERVER_A
        WALLET_A
        univalue
        COMMON_A
        ZEROCOIN_A
        UTIL_A
        BITCOIN_CRYPTO_A
        leveldb
        leveldb_sse42
        memenv
        secp256k1
        ${BerkeleyDB_Cxx_LIBRARY}
        ${OPENSSL_LIBRARIES}
        ${Boost_LIBRARIES}
        ${LibEvent_LIBRARIES}
        )
if(GMP_FOUND)
    target_link_libraries(btcud ${GMP_LIBRARIES})
    target_include_directories(btcud PUBLIC ${GMP_INCLUDE_DIR})
endif()
if(ZeroMQ_FOUND)
    target_link_libraries(btcud ZMQ_A)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        target_link_libraries(btcud 
            ${ZeroMQ_STATIC_LIBRARY}
            )
    else()
        target_link_libraries(btcud 
            ${ZeroMQ_LIBRARY}
            )
    endif()
    target_include_directories(btcud PUBLIC ${ZeroMQ_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/)
endif()
if(MINIUPNP_FOUND)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		target_compile_definitions(btcud
			PUBLIC -DSTATICLIB
			PUBLIC -DMINIUPNP_STATICLIB
		)
	endif()
    target_link_libraries(btcud ${MINIUPNP_LIBRARY})
    target_include_directories(btcud PUBLIC ${MINIUPNP_INCLUDE_DIR})
endif()

add_subdirectory(src/qt)

if(ENABLE_TESTS)
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
        # this is not supported in Windows
        if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
            return()
        endif()

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
    add_dependencies(test_btcu test_btcu_hex leveldb leveldb_sse42 memenv)
    target_include_directories(test_btcu PUBLIC ${BTCU_HEX_DESTDIR} ./src/test)

    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        generate_windows_version_info(test_btcu
            DESCRIPTION "Test lib for ${PACKAGE_NAME}"
        )
        list(APPEND test_btcu_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/test_btcu-res.rc)
    endif()

    target_link_libraries(test_btcu
            SERVER_A
            WALLET_A
            univalue
            COMMON_A
            ZEROCOIN_A
            UTIL_A
            CLI_A
            BITCOIN_CRYPTO_A
            leveldb
            leveldb_sse42
            memenv
            secp256k1
            ${BerkeleyDB_Cxx_LIBRARY}
            ${OPENSSL_LIBRARIES}
            ${Boost_LIBRARIES}
            ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
            ${LibEvent_LIBRARIES}
        )

    if(GMP_FOUND)
        target_link_libraries(test_btcu ${GMP_LIBRARIES})
        target_include_directories(test_btcu PUBLIC ${GMP_INCLUDE_DIR})
    endif()
    if(ZeroMQ_FOUND)
        target_link_libraries(test_btcu ZMQ_A)
        target_include_directories(test_btcu PUBLIC ${ZeroMQ_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp-ethereum/)
        if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
            target_link_libraries(test_btcu 
                ${ZeroMQ_STATIC_LIBRARY}
                )
        else()
            target_link_libraries(test_btcu 
                ${ZeroMQ_LIBRARY}
                )
        endif()
    endif()
    if(MINIUPNP_FOUND)
        if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
            target_compile_definitions(test_btcu
                PUBLIC -DSTATICLIB
                PUBLIC -DMINIUPNP_STATICLIB
            )
        endif()
        target_link_libraries(test_btcu ${MINIUPNP_LIBRARY})
        target_include_directories(test_btcu PUBLIC ${MINIUPNP_INCLUDE_DIR})
    endif()
endif()