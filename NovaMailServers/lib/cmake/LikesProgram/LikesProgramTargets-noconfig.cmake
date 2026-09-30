#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "LikesProgram::LikesProgram" for configuration ""
set_property(TARGET LikesProgram::LikesProgram APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(LikesProgram::LikesProgram PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libLikesProgramStatic.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS LikesProgram::LikesProgram )
list(APPEND _IMPORT_CHECK_FILES_FOR_LikesProgram::LikesProgram "${_IMPORT_PREFIX}/lib/libLikesProgramStatic.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
