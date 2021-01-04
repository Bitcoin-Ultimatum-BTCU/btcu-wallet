add_executable(cpp-ethereum-key KeyAux.h main.cpp)
target_link_libraries(cpp-ethereum-key PRIVATE ethcore devcore Boost::program_options)
install(TARGETS cpp-ethereum-key EXPORT alethTargets DESTINATION bin)
