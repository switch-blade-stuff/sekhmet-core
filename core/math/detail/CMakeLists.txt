
include(${CMAKE_CURRENT_LIST_DIR}/vector/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/quaternion/CMakeLists.txt)

list(APPEND SEK_CORE_INTERFACE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/config.h
        ${CMAKE_CURRENT_LIST_DIR}/policy.hpp
        ${CMAKE_CURRENT_LIST_DIR}/matrix.hpp
        ${CMAKE_CURRENT_LIST_DIR}/matrix2x.hpp
        ${CMAKE_CURRENT_LIST_DIR}/matrix3x.hpp
        ${CMAKE_CURRENT_LIST_DIR}/matrix4x.hpp
        ${CMAKE_CURRENT_LIST_DIR}/sysrandom.hpp
        ${CMAKE_CURRENT_LIST_DIR}/xoroshiro.hpp)

list(APPEND SEK_CORE_PRIVATE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/sysrandom.cpp)