# Copyright (c) 2017 The Bitcoin developers

cmake_minimum_required(VERSION 3.14)
project(secp256k1 LANGUAGES C VERSION 0.1.0)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 90)

include(TestBigEndian)
include(CheckCSourceCompiles)

if (MSVC)
    set(BUILD_SHARED_LIBS OFF)
    set(BUILD_STATIC_LIBS ON)
else()
    set(BUILD_SHARED_LIBS ON)
    set(BUILD_STATIC_LIBS OFF)
endif()

# Add path for custom modules when building as a standalone project
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/script)

# Default to RelWithDebInfo configuration
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Select the configuration for the build" FORCE)
	set(__NO_USER_CMAKE_BUILD_TYPE ON CACHE BOOL "True if the user didn't set a build type on the command line")
endif()

include(AddCompilerFlags)

# libsecp256k1 use a different set of flags.
if(MSVC)
	add_compile_options(/wd4244)
	add_compile_options(/wd4267)
	add_compile_options(/wd4244)
else()
	add_c_compiler_flags(
			-pedantic
			-Wall
			-Wextra
			-Wcast-align
			-Wshadow
			-Wundef
			-Wno-unused-function
			-Wno-overlength-strings
			-Wno-nonnull-compare
			-Wno-implicit-fallthrough
			-std=c89
			-Wnested-externs
			-Wstrict-prototypes
			-Wno-long-long
	)
endif()

#
# Feature Testing
#
check_c_source_compiles([=[
    #include <stdint.h>
    int main(void) {
      uint64_t a = 11, tmp;
      __asm__ __volatile__("movq $0x100000000,%1; mulq %%rsi"
                           : "+a" (a) : "S" (tmp) : "cc", "%rdx");
      return 0;
    }
]=] SECP256K1_HAS_ASM_X64)

test_big_endian(SECP256K1_BIGENDIAN)

check_type_size("__int128" SECP256K1_HAS_INT128)

#
# Defines
#
option(SECP256K1_USE_ENDOMORPHISM "Enable endomorphism" OFF)
if(SECP256K1_USE_ENDOMORPHISM)
	set(USE_ENDOMORPHISM 1)
endif()

if(SECP256K1_HAS_ASM_X64)
  set(USE_ASM_X86_64 1)
endif()

if(SECP256K1_HAS_INT128)
  set(HAVE___INT128 1)
endif()

if(SECP256K1_BIGENDIAN)
  set(WORDS_BIGENDIAN 1)
endif()

if(SECP256K1_HAS_ASM_X64 OR SECP256K1_HAS_ASM_X64)
  set(USE_FIELD_5X52 1)
else()
  set(USE_FIELD_10X26 1)
endif()

if(SECP256K1_HAS_ASM_X64)
  set(USE_SCALAR_4X64 1)
else()
  set(USE_SCALAR_8X32 1)
endif()

if(GMP_FOUND)
  	set(USE_NUM_GMP 1)
  	set(HAVE_LIBGMP 1)
  	set(USE_FIELD_INV_NUM 1)
  	set(USE_SCALAR_INV_NUM 1)
else()
  	set(USE_NUM_NONE 1)
  	set(USE_FIELD_INV_BUILTIN 1)
  	set(USE_SCALAR_INV_BUILTIN 1)
endif()

# Guess the target architecture, within the ones with supported ASM.
# First check if the CMAKE_C_COMPILER_TARGET is set (should be when
# cross compiling), then CMAKE_SYSTEM_PROCESSOR as a fallback if meaningful
# (this is not the case for ARM as the content is highly non standard).
if(CMAKE_C_COMPILER_TARGET MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR SECP256K1_HAS_ASM_X64)
	set(SECP256K1_ASM_BUILD_TARGET "x86_64")
  	set(ASM_BUILD_TARGET "x86_64")
	set(SECP256K1_DEFAULT_USE_ASM ON)
  	set(USE_ASM 1)
elseif(CMAKE_C_COMPILER_TARGET MATCHES "arm-linux-gnueabihf")
	set(SECP256K1_ASM_BUILD_TARGET "arm-linux-gnueabihf")
  	set(ASM_BUILD_TARGET "arm-linux-gnueabihf")
	set(SECP256K1_DEFAULT_USE_ASM ON)
  	set(USE_ASM 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(SECP256K1_DEFAULT_USE_ASM OFF)
  	set(USE_ASM OFF)
endif()

# Enable ASM by default only if we are building for a compatible target.
# The user can still enable/disable it manually if needed.
option(SECP256K1_USE_ASM "Use assembly" ${SECP256K1_DEFAULT_USE_ASM})
if(SECP256K1_USE_ASM)
	macro(unsupported_asm_error)
		message(FATAL_ERROR
			"Assembly is enabled, but not supported for your target architecture."
			"Re-run cmake with -DSECP256K1_USE_ASM=OFF to disable ASM support."
		)
	endmacro()

	if(SECP256K1_ASM_BUILD_TARGET MATCHES "x86_64")
		# We check if amd64 asm is supported.
		check_c_source_compiles("
			#include <stdint.h>
			int main() {
				uint64_t a = 11, tmp;
				__asm__ __volatile__(\"movq \$0x100000000,%1; mulq %%rsi\" : \"+a\"(a) : \"S\"(tmp) : \"cc\", \"%rdx\");
				return 0;
			}
		" USE_ASM_X86_64)

		if(NOT USE_ASM_X86_64)
			unsupported_asm_error()
		endif()
	elseif(SECP256K1_ASM_BUILD_TARGET MATCHES "arm-linux-gnueabihf")
		enable_language(ASM)
		set(USE_EXTERNAL_ASM 1)
		add_library(secp256k1_common src/asm/field_10x26_arm.s)
		target_link_libraries(secp256k1 secp256k1_common)
	else()
		unsupported_asm_error()
	endif()
endif()

set(SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY "" CACHE STRING "Test-only override of the (autodetected by the C code) \"widemul\" setting (can be int64 or int128)")
if(SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY STREQUAL "int128")
	message(STATUS "Force the use of the (unsigned) __int128 based wide multiplication implementation")
	target_compile_definitions(secp256k1 PUBLIC USE_FORCE_WIDEMUL_INT128=1)
  	set(USE_FORCE_WIDEMUL_INT128 1)
elseif(SECP256K1_TEST_OVERRIDE_WIDE_MULTIPLY STREQUAL "int64")
	message(STATUS "Force the use of the (u)int64_t based wide multiplication implementation")
	target_compile_definitions(secp256k1 PUBLIC USE_FORCE_WIDEMUL_INT64=1)
  	set(USE_FORCE_WIDEMUL_INT64 1)
endif()

option(SECP256K1_BUILD_TEST "Build secp256k1's unit tests" OFF)
include(CMakeDependentOption)
cmake_dependent_option(
	SECP256K1_BUILD_OPENSSL_TESTS "Build the OpenSSL tests" OFF
	SECP256K1_BUILD_TEST OFF
)
if(SECP256K1_BUILD_OPENSSL_TESTS)
	if(NOT OpenSSL_FOUND)
		message(FATAL_ERROR
			"OpenSSL is not found, but is required for some tests. You can disable them by passing -DSECP256K1_BUILD_OPENSSL_TESTS=OFF."
		)
	endif()
	set(ENABLE_OPENSSL_TESTS 1)
endif()

# External default callbacks
option(SECP256K1_ENABLE_EXTERNAL_DEFAULT_CALLBACKS "Enable external default callbacks" OFF)
if(SECP256K1_ENABLE_EXTERNAL_DEFAULT_CALLBACKS)
  	set(USE_EXTERNAL_DEFAULT_CALLBACKS 1)
endif()

# Make the emult window size customizable.
set(SECP256K1_ECMULT_WINDOW_SIZE 15 CACHE STRING "Window size for ecmult precomputation for verification, specified as integer in range [2..24].")
              
if(${SECP256K1_ECMULT_WINDOW_SIZE} LESS 2 OR ${SECP256K1_ECMULT_WINDOW_SIZE} GREATER 24)
	message(FATAL_ERROR "SECP256K1_ECMULT_WINDOW_SIZE must be an integer in range [2..24]")
else()
  	set(ECMULT_WINDOW_SIZE 1)
endif()

set(SECP256K1_ECMULT_GEN_PRECISION 4 CACHE STRING "Precision bits to tune the precomputed table size for signing.")
set(VALID_PRECISIONS 2 4 8)
if(NOT ${SECP256K1_ECMULT_GEN_PRECISION} IN_LIST VALID_PRECISIONS)
	message(FATAL_ERROR "SECP256K1_ECMULT_GEN_PRECISION not 2, 4, 8")
else()
  	set(SECP256K1_ECMULT_GEN_PRECISION ${SECP256K1_ECMULT_GEN_PRECISION})
endif()

include_directories(
	.
	src
	# For the config
	${CMAKE_CURRENT_BINARY_DIR}/src
)

# The library
add_library(secp256k1 src/secp256k1.c)
target_include_directories(secp256k1 PUBLIC include)

set(SECP256K1_PUBLIC_HEADERS
	include/secp256k1.h
	include/secp256k1_sha256.h
)

if(GMP_FOUND)
	message(STATUS "GMP included into secp256k1")
	target_link_libraries(secp256k1 ${GMP_LIBRARIES})
    target_include_directories(secp256k1 PUBLIC ${GMP_INCLUDE_DIR})
endif()

# Executable internal to secp256k1 need to have the HAVE_CONFIG_H define set.
# For convenience, we wrap this into a function.
function(link_secp256k1_internal NAME)
	target_link_libraries(${NAME} secp256k1)
	target_compile_definitions(${NAME} PRIVATE HAVE_CONFIG_H SECP256K1_BUILD)
	if(SECP256K1_BUILD_OPENSSL_TESTS)
		target_link_libraries(${NAME} OpenSSL::Crypto)
	endif()
endfunction(link_secp256k1_internal)

include(InstallationHelper)

# Phony target to build benchmarks
add_custom_target(bench-secp256k1)

function(add_secp256k1_bench NAME)
	# currently unavailable
	return()

	set(EXECUTABLE_NAME "${NAME}-bench")
	add_executable(${EXECUTABLE_NAME} ${ARGN})
	link_secp256k1_internal(${EXECUTABLE_NAME})

	set(BENCH_NAME "bench-secp256k1-${NAME}")
	add_custom_target(${BENCH_NAME}
		COMMENT "Benchmarking libsecp256k1 ${NAME}"
		COMMAND ${EXECUTABLE_NAME}
		USES_TERMINAL
	)
	add_dependencies(bench-secp256k1 ${BENCH_NAME})

	install_target("${EXECUTABLE_NAME}"
		COMPONENT secp256k1-bench
		EXCLUDE_FROM_ALL
	)
endfunction(add_secp256k1_bench)

# ECDH module
option(SECP256K1_ENABLE_MODULE_ECDH "Build libsecp256k1's ECDH module" ON)
if(SECP256K1_ENABLE_MODULE_ECDH)
	set(ENABLE_MODULE_ECDH 1)
	add_secp256k1_bench(ecdh src/bench_ecdh.c)
	list(APPEND SECP256K1_PUBLIC_HEADERS include/secp256k1_ecdh.h)
endif()

# Recovery module
option(SECP256K1_ENABLE_MODULE_RECOVERY "Build libsecp256k1's recovery module" ON)
if(SECP256K1_ENABLE_MODULE_RECOVERY)
	set(ENABLE_MODULE_RECOVERY 1)
	add_secp256k1_bench(recover src/bench_recover.c)
	list(APPEND SECP256K1_PUBLIC_HEADERS include/secp256k1_recovery.h)
endif()

# temporary unavailable
option(SECP256K1_ECMULT_STATIC_PRECOMPUTATION "Precompute libsecp256k1's elliptic curve multiplication tables" OFF)
if(SECP256K1_ECMULT_STATIC_PRECOMPUTATION)
	set(USE_ECMULT_STATIC_PRECOMPUTATION 1)

	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		set(USE_ECMULT_STATIC_PRECOMPUTATION 0)
		add_executable(gen_context src/gen_context.c)
		target_include_directories(gen_context PRIVATE ${CMAKE_SOURCE_DIR})

		add_custom_target(ecmult_static_context gen_context WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		add_dependencies(secp256k1 ecmult_static_context)

		install(TARGETS secp256k1 ARCHIVE DESTINATION lib)
		install(DIRECTORY include/ DESTINATION include)
	else()
		include(NativeExecutable)
		add_native_executable(gen_context src/gen_context.c)

		add_custom_command(
			OUTPUT ecmult_static_context.h
			COMMAND gen_context
		)

		target_sources(secp256k1 PRIVATE ecmult_static_context.h)
	endif()
else()
	set(USE_ECMULT_STATIC_PRECOMPUTATION 0)
endif()
	
# If this project is not the top level project, then don't install by default
get_directory_property(SECP256K1_PARENT_DIRECTORY PARENT_DIRECTORY)
if(SECP256K1_PARENT_DIRECTORY)
  set(SECP256K1_INSTALL_EXCLUDE_FROM_ALL EXCLUDE_FROM_ALL)
endif()

if(BUILD_SHARED_LIBS)
	install_shared_library(secp256k1
		PUBLIC_HEADER ${SECP256K1_PUBLIC_HEADERS}
		${SECP256K1_INSTALL_EXCLUDE_FROM_ALL}
	)
else()
	set_property(TARGET secp256k1 PROPERTY PUBLIC_HEADER ${SECP256K1_PUBLIC_HEADERS})
	install_target(secp256k1 ${SECP256K1_INSTALL_EXCLUDE_FROM_ALL})
endif()

# Generate the config
configure_file(src/libsecp256k1-config.h.cmake.in src/libsecp256k1-config.h ESCAPE_QUOTES)
target_compile_definitions(secp256k1 PRIVATE HAVE_CONFIG_H SECP256K1_BUILD)

# Build the Java binding
option(SECP256K1_ENABLE_JNI "Enable the Java Native Interface binding" OFF)
if(SECP256K1_ENABLE_JNI)
	if(NOT SECP256K1_ENABLE_MODULE_ECDH)
		message(FATAL_ERROR "The secp256k1 JNI support requires ECDH. Try again with -DSECP256K1_ENABLE_MODULE_ECDH=ON.")
	endif()

	find_package(Java REQUIRED)
	find_package(JNI REQUIRED)
	include(UseJava)

	add_library(secp256k1_jni SHARED
		src/java/org_bitcoin_NativeSecp256k1.c
		src/java/org_bitcoin_Secp256k1Context.c
	)

	install_shared_library(secp256k1_jni ${SECP256K1_INSTALL_EXCLUDE_FROM_ALL})

	target_include_directories(secp256k1_jni PUBLIC ${JNI_INCLUDE_DIRS})
	# As per CMake documentation: the POSITION_INDEPENDENT_CODE property is set
	# when a target is created. It defaults to True for SHARED or MODULE library
	# targets and False otherwise.
	# The secp256ki_jni library being shared, the property is set and it will
	# build with PIC enabled. But the secp256k1 dependency might not have the
	# property set, so it's associated source files won't be built with PIC
	# enabled. That would cause the linker to fail.
	# Forcing the property for the secp256k1 library fixes the issue.
	set_target_properties(secp256k1 PROPERTIES POSITION_INDEPENDENT_CODE ON)
	link_secp256k1_internal(secp256k1_jni)
endif()

# Tests
if(SECP256K1_BUILD_TEST)
	
	# RPATH is not supported in windows
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		return()
	endif()

	include(TestSuite)
	create_test_suite(secp256k1)

	function(create_secp256k1_test NAME FILES)
		add_test_to_suite(secp256k1 ${NAME} EXCLUDE_FROM_ALL ${FILES})
		link_secp256k1_internal(${NAME})
	endfunction()

	create_secp256k1_test(secp256k1-tests src/tests.c)
	create_secp256k1_test(secp256k1-exhaustive_tests src/tests_exhaustive.c)

	target_compile_definitions(secp256k1-tests PRIVATE VERIFY)
	target_compile_definitions(secp256k1-exhaustive_tests PRIVATE VERIFY)

	if(SECP256K1_ENABLE_JNI)
		set(SECP256k1_JNI_TEST_JAR "secp256k1-jni-test")

		set(CMAKE_JNI_TARGET TRUE)
		add_jar(secp256k1-jni-test-jar
			SOURCES
				src/java/org/bitcoin/NativeSecp256k1.java
				src/java/org/bitcoin/NativeSecp256k1Test.java
				src/java/org/bitcoin/NativeSecp256k1Util.java
				src/java/org/bitcoin/Secp256k1Context.java
			ENTRY_POINT org/bitcoin/NativeSecp256k1Test
			OUTPUT_NAME "${SECP256k1_JNI_TEST_JAR}"
		)
		add_dependencies(secp256k1-jni-test-jar secp256k1_jni)

		add_custom_target(check-secp256k1-java
			COMMAND
				"${Java_JAVA_EXECUTABLE}"
				"-Djava.library.path=${CMAKE_CURRENT_BINARY_DIR}"
				"-jar"
				"${SECP256k1_JNI_TEST_JAR}.jar"
			WORKING_DIRECTORY
				"${CMAKE_CURRENT_BINARY_DIR}"
		)
		add_dependencies(check-secp256k1-java secp256k1-jni-test-jar)
		add_dependencies(check-secp256k1 check-secp256k1-java)
	endif()
endif(SECP256K1_BUILD_TEST)

# Benchmarks
add_secp256k1_bench(verify src/bench_verify.c)
add_secp256k1_bench(sign src/bench_sign.c)
add_secp256k1_bench(internal src/bench_internal.c)
