# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles/CachyOsTools_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/CachyOsTools_autogen.dir/ParseCache.txt"
  "CachyOsTools_autogen"
  )
endif()
