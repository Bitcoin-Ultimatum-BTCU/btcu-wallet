#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libscrypt::scrypt" for configuration ""
set_property(TARGET libscrypt::scrypt APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(libscrypt::scrypt PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_DIR}/libscrypt.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS libscrypt::scrypt )
list(APPEND _IMPORT_CHECK_FILES_FOR_libscrypt::scrypt "${_DIR}/libscrypt.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
