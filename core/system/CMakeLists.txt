
include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)

list(APPEND SEK_CORE_INTERFACE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/native_file.hpp
        ${CMAKE_CURRENT_LIST_DIR}/native_filemap.hpp
        ${CMAKE_CURRENT_LIST_DIR}/char_file.hpp
        ${CMAKE_CURRENT_LIST_DIR}/clipboard.hpp)