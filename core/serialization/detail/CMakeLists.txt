
include(${CMAKE_CURRENT_LIST_DIR}/json/CMakeLists.txt)

target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/util.hpp
        ${CMAKE_CURRENT_LIST_DIR}/manipulators.hpp
        ${CMAKE_CURRENT_LIST_DIR}/traits.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ranges.hpp
        ${CMAKE_CURRENT_LIST_DIR}/tuples.hpp
        ${CMAKE_CURRENT_LIST_DIR}/archive_error.hpp
        ${CMAKE_CURRENT_LIST_DIR}/archive_reader.hpp
        ${CMAKE_CURRENT_LIST_DIR}/archive_writer.hpp
        ${CMAKE_CURRENT_LIST_DIR}/binary_archive.hpp)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/archive_error.cpp)