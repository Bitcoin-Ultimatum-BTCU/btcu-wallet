set(
    sources
    AccountManager.cpp AccountManager.h
    main.cpp
    MinerAux.cpp MinerAux.h
)

add_executable(cpp-ethereum ${sources})
target_link_libraries(
    cpp-ethereum
    PRIVATE ethereum ethashseal evm web3jsonrpc webthree devcore Boost::program_options
)

target_include_directories(cpp-ethereum PRIVATE ../utils)

if(MINIUPNPC)
    target_compile_definitions(cpp-ethereum PRIVATE ETH_MINIUPNPC)
endif()

install(TARGETS cpp-ethereum DESTINATION bin)
