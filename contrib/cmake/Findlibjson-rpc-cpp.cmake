# Find libjson-rpc-cpp
#
# Find the libjson-rpc-cpp includes and library
# 
# if you nee to add a custom library search path, do it via via CMAKE_PREFIX_PATH 
# 
# This module defines
#  JSONRPCCPP_INCLUDE_DIRS, where to find header, etc.
#  JSONRPCCPP_LIBRARIES, the libraries needed to use jsoncpp.
#  JSONRPCCPP_FOUND, If false, do not try to use jsoncpp.

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        return()
endif()


set(JSONRPCCPP_INCLUDE_DIR "/usr/include/jsonrpccpp")
set(JSONRPCCPP_LIBRARY_DIRS "/usr/lib/x86_64-linux-gnu")

find_path(
    ${JSONRPCCPP_INCLUDE_DIR}/common
    NAMES errors.h
    DOC "libjson-rpc-cpp common include dir"
)

find_library(
    JSONRPCCPP_LIBRARY_DIRS
    NAMES libjsonrpccpp-common
    DOC "libjson-rpc-cpp common library"
)

find_path(
    ${JSONRPCCPP_INCLUDE_DIR}/client
    NAMES client.h
    DOC "libjson-rpc-cpp client include dir"
)

find_library(
    JSONRPCCPP_LIBRARY_DIRS
    NAMES libjsonrpccpp-client
    DOC "libjson-rpc-cpp client library"
)

find_path(
    JSONRPCCPP_INCLUDE_DIR
    NAMES server.h
    DOC "libjson-rpc-cpp server include dir"
)

find_library(
    JSONRPCCPP_LIBRARY_DIRS
    NAMES libjsonrpccpp-server
    DOC "libjson-rpc-cpp server library"
)


set(JSONRPCCPP_INCLUDE_DIRS ${JSONRPCCPP_INCLUDE_DIR})
set(JSONRPCCPP_LIBRARIES ${common_LIBRARY} ${server_LIBRARY} ${client_LIBRARY})

# handle the QUIETLY and REQUIRED arguments and set JSONRPCCPP_FOUND to TRUE
# if all listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libjson-rpc-cpp 
                        REQUIRED_VARS
                                JSONRPCCPP_INCLUDE_DIR
                        HANDLE_COMPONENTS
                        )
mark_as_advanced (JSONRPCCPP_INCLUDE_DIR JSONRPCCPP_LIBRARY)

if(JSONRPCCPP_INCLUDE_DIR)
	include(ExternalLibraryHelper)
	find_component(libjson-rpc-cpp libjson-rpc-cpp 
		NAMES libjson-rpc-cpp 
		PATHS ${JSONRPCCPP_INCLUDE_DIR}
		INCLUDE_DIRS ${JSONRPCCPP_INCLUDE_DIR}
	)
endif()
