
include(${CMAKE_CURRENT_LIST_DIR}/unix/CMakeLists.txt)

target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/native_file.hpp
        ${CMAKE_CURRENT_LIST_DIR}/native_filemap.hpp
        ${CMAKE_CURRENT_LIST_DIR}/char_file.hpp
        ${CMAKE_CURRENT_LIST_DIR}/clipboard.hpp)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/native_file.cpp
        ${CMAKE_CURRENT_LIST_DIR}/native_filemap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/char_file.cpp
        ${CMAKE_CURRENT_LIST_DIR}/clipboard.cpp)