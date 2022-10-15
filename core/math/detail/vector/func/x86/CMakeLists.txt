
target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/common.hpp
        ${CMAKE_CURRENT_LIST_DIR}/arithmetic.hpp
        ${CMAKE_CURRENT_LIST_DIR}/bitwise.hpp
        ${CMAKE_CURRENT_LIST_DIR}/category.hpp
        ${CMAKE_CURRENT_LIST_DIR}/geometric.hpp

        ${CMAKE_CURRENT_LIST_DIR}/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/f32/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/f64/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/i32/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/i64/utility.hpp
        ${CMAKE_CURRENT_LIST_DIR}/relational.hpp
        ${CMAKE_CURRENT_LIST_DIR}/f32/relational.hpp
        ${CMAKE_CURRENT_LIST_DIR}/f64/relational.hpp
        ${CMAKE_CURRENT_LIST_DIR}/i32/relational.hpp
        ${CMAKE_CURRENT_LIST_DIR}/i64/relational.hpp
        ${CMAKE_CURRENT_LIST_DIR}/exponential.hpp
        ${CMAKE_CURRENT_LIST_DIR}/f32/exponential.hpp
        ${CMAKE_CURRENT_LIST_DIR}/f64/exponential.hpp
        ${CMAKE_CURRENT_LIST_DIR}/trigonometric.hpp
        ${CMAKE_CURRENT_LIST_DIR}/f32/trigonometric.hpp
        ${CMAKE_CURRENT_LIST_DIR}/f64/trigonometric.hpp)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/utility.cpp
        ${CMAKE_CURRENT_LIST_DIR}/f32/exponential.cpp
        ${CMAKE_CURRENT_LIST_DIR}/f32/trigonometric.cpp
        ${CMAKE_CURRENT_LIST_DIR}/f64/exponential.cpp
        ${CMAKE_CURRENT_LIST_DIR}/f64/trigonometric.cpp)