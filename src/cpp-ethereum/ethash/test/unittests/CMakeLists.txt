# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(GTest REQUIRED)
else()
    find_package(GTest CONFIG REQUIRED)
endif()
include_directories(${Boost_INCLUDE_DIRS})

add_executable(
    ethash-test
    helpers.hpp
    test_bit_manipulation.cpp
    test_cases.hpp
    test_ethash.cpp
    test_keccak.cpp
    test_kiss.cpp
    test_managed.cpp
    test_primes.cpp
    test_progpow.cpp
)

target_link_libraries(ethash-test 
    PRIVATE 
        ethash 
        ${Boost_LIBRARIES}
        )
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(ethash-test 
        PRIVATE 
            GTest::GTest 
            GTest::Main
            )
else()
    target_link_libraries(ethash-test 
        PRIVATE 
            GTest::gtest 
            GTest::gtest_main
            )
endif()
target_include_directories(ethash-test PRIVATE ${ETHASH_PRIVATE_INCLUDE_DIR})
set_target_properties(ethash-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ..)
