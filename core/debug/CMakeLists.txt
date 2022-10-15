
include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)

target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/assert.hpp
        ${CMAKE_CURRENT_LIST_DIR}/logger.hpp)
