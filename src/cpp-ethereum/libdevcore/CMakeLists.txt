file(GLOB sources "*.cpp")
file(GLOB headers "*.h")

add_library(devcore ${sources} ${headers})
set(ENABLE_PRECOMPILED_HEADERS OFF)

# Needed to prevent including system-level boost headers:
target_include_directories(devcore SYSTEM PUBLIC ${Boost_INCLUDE_DIR} PRIVATE ../utils ../ethash/include)


target_link_libraries(devcore PUBLIC cpp-ethereum-buildinfo Boost::filesystem Boost::system Boost::log Boost::thread Threads::Threads PRIVATE ethash::ethash)

if (MSVC)
    find_package(LevelDB)
    target_include_directories(devcore SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS} ../ethash/include)
    target_link_libraries(devcore PRIVATE ${LEVELDB_LIBRARIES})
else()
    find_package(leveldb CONFIG REQUIRED)
    target_include_directories(devcore SYSTEM PUBLIC ../ethash/include)
    target_link_libraries(devcore PRIVATE leveldb::leveldb)
endif()

# find_package(RocksDB REQUIRED)
target_link_libraries(devcore PRIVATE RocksDB::rocksdb)
