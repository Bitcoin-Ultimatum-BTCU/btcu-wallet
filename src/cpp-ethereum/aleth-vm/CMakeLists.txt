add_executable(cpp-ethereum-vm main.cpp)

target_link_libraries(cpp-ethereum-vm PRIVATE ethereum evm ethashseal devcore Boost::program_options)

target_include_directories(cpp-ethereum-vm PRIVATE ../utils)

install(TARGETS cpp-ethereum-vm EXPORT alethTargets DESTINATION bin)
