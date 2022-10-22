
include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)

list(APPEND SEK_CORE_INTERFACE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/base64.hpp
        ${CMAKE_CURRENT_LIST_DIR}/binary.hpp
        ${CMAKE_CURRENT_LIST_DIR}/json.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ubjson.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ranges.hpp
        ${CMAKE_CURRENT_LIST_DIR}/tuples.hpp
        ${CMAKE_CURRENT_LIST_DIR}/generic.hpp)