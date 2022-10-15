
include(${CMAKE_CURRENT_LIST_DIR}/func/CMakeLists.txt)

target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/fwd.hpp
        ${CMAKE_CURRENT_LIST_DIR}/storage.hpp
        ${CMAKE_CURRENT_LIST_DIR}/shuffle.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type.hpp)