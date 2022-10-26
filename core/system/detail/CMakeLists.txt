
include(${CMAKE_CURRENT_LIST_DIR}/unix/CMakeLists.txt)

list(APPEND SEK_CORE_INTERFACE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/file_mode.hpp)

list(APPEND SEK_CORE_PRIVATE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/native_file.cpp
        ${CMAKE_CURRENT_LIST_DIR}/native_filemap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/char_file.cpp
        ${CMAKE_CURRENT_LIST_DIR}/clipboard.cpp)