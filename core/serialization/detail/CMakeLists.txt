
include(${CMAKE_CURRENT_LIST_DIR}/json/CMakeLists.txt)

list(APPEND SEK_CORE_INTERFACE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/util.hpp
        ${CMAKE_CURRENT_LIST_DIR}/manipulators.hpp
        ${CMAKE_CURRENT_LIST_DIR}/traits.hpp
        ${CMAKE_CURRENT_LIST_DIR}/archive_error.hpp
        ${CMAKE_CURRENT_LIST_DIR}/archive_reader.hpp
        ${CMAKE_CURRENT_LIST_DIR}/archive_writer.hpp
        ${CMAKE_CURRENT_LIST_DIR}/binary_archive.hpp)

list(APPEND SEK_CORE_PRIVATE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/archive_error.cpp)