# .rst:
# FindRocksDB
# --------------
# Find the RocksDB libraries
#
# The following variables are optionally searched for defaults
#  ROCKSDB_ROOT_DIR:    Base directory where all RocksDB components are found
#
# The following are set after configuration is done:
#  RocksDB_FOUND
#  RocksDB_INCLUDE_DIR
#  ROCKSDB_LIBRARY

find_brew_prefix(_ROCKSDB_BREW_HINT rocksdb)

if(ROCKSDB_ROOT_DIR)
    set(_ROCKSDB_PATHS "${ROCKSDB_ROOT_DIR}")
else()
    # Paths for anything other than Windows
    # Cellar/rocksdb is for macOS from homebrew installation
    list(APPEND _ROCKSDB_PATHS
            "/usr/local/opt/rocksdb"
            "/usr/local/Cellar/rocksdb"
            "/opt"
            "/opt/local"
			"/usr"
            "/usr/local"
            )
endif()
if(NOT RocksDB_INCLUDE_DIR)
    find_path(RocksDB_INCLUDE_DIR NAMES rocksdb/db.h
            PATHS "${_ROCKSDB_PATHS}"
		    HINTS ${_ROCKSDB_BREW_HINT}
            PATH_SUFFIXES "include" "includes")
endif()

if(RocksDB_INCLUDE_DIR)

	find_component(
		RocksDB rocksdb
		NAMES
            rocksdbd rocksdb librocksdb
		INCLUDE_DIRS ${RocksDB_INCLUDE_DIR}
		HINTS ${_ROCKSDB_BREW_HINT}
        PATH_SUFFIXES "lib" "lib64" "libs" "libs64"
        PATHS ${_ROCKSDB_PATHS}
	)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RocksDB
	REQUIRED_VARS RocksDB_INCLUDE_DIR
	HANDLE_COMPONENTS
)

if(RocksDB_FOUND)
  set(ZeroMQ_INCLUDE_DIR "${RocksDB_INCLUDE_DIRS}")
  set(ROCKSDB_LIBRARY "${RocksDB_LIBRARIES}")

  message(STATUS "Found RocksDB  (include: ${RocksDB_INCLUDE_DIR}, library: ${ROCKSDB_LIBRARY})")
  mark_as_advanced(RocksDB_INCLUDE_DIR ROCKSDB_LIBRARY)
endif()
