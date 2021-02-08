if(${CURVE} STREQUAL "BN128")
  project(zm)

  include_directories(ate-pairing/include)
  include_directories(xbyak)
  add_library(
    zm

    ate-pairing/src/zm.cpp
    ate-pairing/src/zm2.cpp
  )
endif()
