# Aleth: Ethereum C++ client, tools and libraries.
# Copyright 2018 Aleth Autors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.

set(
    sources
    interpreter.h
    VM.cpp
    VM.h
    VMCalls.cpp
    VMConfig.h
    VMOpt.cpp
)
add_library(cpp-ethereum-interpreter STATIC ${sources})
target_include_directories(cpp-ethereum-interpreter PRIVATE "${UTILS_INCLUDE_DIR}" ../ethash/include)
target_link_libraries(cpp-ethereum-interpreter PRIVATE devcore cpp-ethereum-buildinfo evmc::evmc evmc::instructions)

if(ALETH_INTERPRETER_SHARED)
    # Build cpp-ethereum-interpreter additionally as a shared library to include it in the package
    add_library(cpp-ethereum-interpreter-shared SHARED ${sources})
    target_link_libraries(cpp-ethereum-interpreter-shared PRIVATE devcore cpp-ethereum-buildinfo evmc::evmc evmc::instructions)
    set_target_properties(cpp-ethereum-interpreter-shared PROPERTIES OUTPUT_NAME cpp-ethereum-interpreter)
    install(TARGETS cpp-ethereum-interpreter-shared EXPORT alethTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(cpp-ethereum-interpreter PRIVATE -fstack-usage)
endif()
