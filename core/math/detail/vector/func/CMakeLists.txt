
include(${CMAKE_CURRENT_LIST_DIR}/fwd/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/x86/CMakeLists.txt)

list(APPEND SEK_CORE_INTERFACE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/arithmetic.hpp
        ${CMAKE_CURRENT_LIST_DIR}/bitwise.hpp
        ${CMAKE_CURRENT_LIST_DIR}/category.hpp
        ${CMAKE_CURRENT_LIST_DIR}/exponential.hpp
        ${CMAKE_CURRENT_LIST_DIR}/geometric.hpp
        ${CMAKE_CURRENT_LIST_DIR}/relational.hpp
        ${CMAKE_CURRENT_LIST_DIR}/trigonometric.hpp
        ${CMAKE_CURRENT_LIST_DIR}/utility.hpp)