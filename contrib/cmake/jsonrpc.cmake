if (NOT TARGET jsoncpp_lib)
    # Include jsoncpp dependency if not included yet.
    # add_subdirectory(${CMAKE_SOURCE_DIR}/src/jsoncpp/jsoncpp)
endif()

# HTTP client from JSON RPC CPP requires curl library. It can find it itself,
# but we need to know the libcurl location for static linking.

set(CURL_NO_CURL_CMAKE OFF)
find_package(CURL REQUIRED)
if(CURL_FOUND)
    message(STATUS "Found CURL")
    
    message(STATUS CURL_LIBRARY:)
    message (STATUS ${CURL_LIBRARY})
    message(STATUS CURL_LIBRARIES:)
    message (STATUS ${CURL_LIBRARIES})
    message(STATUS CURL_INCLUDE_DIRS:)
    message (STATUS ${CURL_INCLUDE_DIRS})
else()
    message(WARNING "CURL not found! JsonRPC requires this functionality")
endif()

# HTTP server from JSON RPC CPP requires microhttpd library. It can find it itself,
# but we need to know the MHD location for static linking.
find_package(MHD REQUIRED)

set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
               # Build static lib but suitable to be included in a shared lib.
               -DCMAKE_POSITION_INDEPENDENT_CODE=On
               -DBUILD_STATIC_LIBS=On
               -DBUILD_SHARED_LIBS=Off
               -DUNIX_DOMAIN_SOCKET_SERVER=Off
               -DUNIX_DOMAIN_SOCKET_CLIENT=Off
               -DHTTP_SERVER=On
               -DHTTP_CLIENT=On
               -DCOMPILE_TESTS=Off
               -DCOMPILE_STUBGEN=Off
               -DCOMPILE_EXAMPLES=Off
               # Point to jsoncpp library.
               -DJSONCPP_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/src/jsoncpp/jsoncpp/include
               -DCURL_INCLUDE_DIR=${CURL_INCLUDE_DIRS}
               -DCURL_LIBRARY=${CURL_LIBRARY}
               -DMHD_INCLUDE_DIR=${MHD_INCLUDE_DIR}
               -DMHD_LIBRARY=${MHD_LIBRARY})
if(${CMAKE_BUILD_TYPE} MATCHES "DEBUG" OR ${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(CMAKE_ARGS ${CMAKE_ARGS}
               -DCMAKE_BUILD_TYPE=)
else()
    set(CMAKE_ARGS ${CMAKE_ARGS}
               -DCMAKE_BUILD_TYPE=Release)
endif()

if (MSVC)
    set(CMAKE_ARGS ${CMAKE_ARGS}
            -DJSONCPP_LIBRARY=jsoncpp_lib_static
            -DJSONCPP_LIBRARY_DEBUG=jsoncpp_lib_static
            -DCURL_LIBRARY_DEBUG=${CURL_LIBRARIES}
            -DMHD_LIBRARY_DEBUG=${MHD_LIBRARY}
            -DWINVER=0x0A00
            -D_WIN32_WINNT=0x0A00)
elseif (WIN32)
    set(CMAKE_ARGS ${CMAKE_ARGS}
            -DJSONCPP_LIBRARY=jsoncpp_lib
            -DCURL_LIBRARY_DEBUG=${CURL_LIBRARY}
            -DMHD_LIBRARY_DEBUG=${MHD_LIBRARY}
            -DWINVER=0x0A00
            -D_WIN32_WINNT=0x0A00)
else()
    set(CMAKE_ARGS ${CMAKE_ARGS}
            # Select jsoncpp include prefix: <json/...> or <jsoncpp/json/...>
            -DJSONCPP_LIBRARY=jsoncpp_lib)
endif()

include(ExternalProject)
if(${CMAKE_BUILD_TYPE} MATCHES "DEBUG" OR ${CMAKE_BUILD_TYPE} MATCHES "Debug")
    ExternalProject_Add(jsonrpc-project
        PREFIX deps/jsonrpc
        URL https://github.com/cinemast/libjson-rpc-cpp/archive/v0.7.0.tar.gz
        URL_HASH SHA256=669c2259909f11a8c196923a910f9a16a8225ecc14e6c30e2bcb712bab9097eb
        # On Windows it tries to install this dir. Create it to prevent failure.
        PATCH_COMMAND cmake -E make_directory <SOURCE_DIR>/win32-deps/include
        CMAKE_ARGS ${CMAKE_ARGS}
        # Overwtire build and install commands to force Release build on MSVC.
        BUILD_COMMAND cmake --build <BINARY_DIR> --config Debug
        INSTALL_COMMAND cmake --build <BINARY_DIR> --config Debug --target install
    )
else()
    ExternalProject_Add(jsonrpc-project
        PREFIX deps/jsonrpc
        URL https://github.com/cinemast/libjson-rpc-cpp/archive/v0.7.0.tar.gz
        URL_HASH SHA256=669c2259909f11a8c196923a910f9a16a8225ecc14e6c30e2bcb712bab9097eb
        # On Windows it tries to install this dir. Create it to prevent failure.
        PATCH_COMMAND cmake -E make_directory <SOURCE_DIR>/win32-deps/include
        CMAKE_ARGS ${CMAKE_ARGS}
        # Overwtire build and install commands to force Release build on MSVC.
        BUILD_COMMAND cmake --build <BINARY_DIR> --config Release
        INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install
    )
endif()

if (MSVC)
    add_dependencies(jsonrpc-project jsoncpp_lib_static)
else()
    add_dependencies(jsonrpc-project jsoncpp_lib)
endif()

# Create libjson-rpc-cpp imported libraries
if (WIN32)
    # On Windows CMAKE_INSTALL_PREFIX is ignored and installs to dist dir.
    ExternalProject_Get_Property(jsonrpc-project BINARY_DIR)
    set(INSTALL_DIR ${BINARY_DIR}/dist)
else()
    ExternalProject_Get_Property(jsonrpc-project INSTALL_DIR)
endif()
set(JSONRPC_INCLUDE_DIR ${INSTALL_DIR}/include)
file(MAKE_DIRECTORY ${JSONRPC_INCLUDE_DIR})  # Must exist.

add_library(libjson-rpc-cpp::common STATIC IMPORTED)
set_property(TARGET libjson-rpc-cpp::common PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}jsonrpccpp-common${CMAKE_STATIC_LIBRARY_SUFFIX})
if (MSVC)
    set_property(TARGET libjson-rpc-cpp::common PROPERTY INTERFACE_LINK_LIBRARIES jsoncpp_lib_static)
else()
    set_property(TARGET libjson-rpc-cpp::common PROPERTY INTERFACE_LINK_LIBRARIES jsoncpp_lib)
endif()
set_property(TARGET libjson-rpc-cpp::common PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${JSONRPC_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src/jsoncpp/jsoncpp/include)
add_dependencies(libjson-rpc-cpp::common jsonrpc-project)

add_library(libjson-rpc-cpp::client STATIC IMPORTED)
set_property(TARGET libjson-rpc-cpp::client PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}jsonrpccpp-client${CMAKE_STATIC_LIBRARY_SUFFIX})
target_link_libraries(libjson-rpc-cpp::client INTERFACE libjson-rpc-cpp::common ${CURL_LIBRARIES})
set_property(TARGET libjson-rpc-cpp::client PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CURL_INCLUDE_DIRS})
add_dependencies(libjson-rpc-cpp::client jsonrpc-project)

add_library(libjson-rpc-cpp::server STATIC IMPORTED)
set_property(TARGET libjson-rpc-cpp::server PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}jsonrpccpp-server${CMAKE_STATIC_LIBRARY_SUFFIX})
set_property(TARGET libjson-rpc-cpp::server PROPERTY INTERFACE_LINK_LIBRARIES libjson-rpc-cpp::common ${MHD_LIBRARY})
set_property(TARGET libjson-rpc-cpp::server PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MHD_INCLUDE_DIR})
add_dependencies(libjson-rpc-cpp::server jsonrpc-project)

unset(INSTALL_DIR)
unset(CMAKE_ARGS)