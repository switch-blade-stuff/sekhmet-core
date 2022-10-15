
target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/native_file.hpp
        ${CMAKE_CURRENT_LIST_DIR}/native_filemap.hpp)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/native_file.cpp
        ${CMAKE_CURRENT_LIST_DIR}/native_filemap.cpp)