set(
    sources
    main.cpp
)

add_executable(cpp-ethereum-bootnode ${sources})
target_link_libraries(
    cpp-ethereum-bootnode
    PRIVATE p2p devcore Boost::program_options
)

install(TARGETS cpp-ethereum-bootnode DESTINATION bin)
