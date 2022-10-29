
list(APPEND SEK_CORE_INTERFACE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/fwd.hpp
        ${CMAKE_CURRENT_LIST_DIR}/traits.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_error.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_data.hpp
        ${CMAKE_CURRENT_LIST_DIR}/any.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_factory.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_info.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_db.hpp)

list(APPEND SEK_CORE_PRIVATE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/type_error.cpp
        ${CMAKE_CURRENT_LIST_DIR}/any.cpp
        ${CMAKE_CURRENT_LIST_DIR}/type_info.cpp
        ${CMAKE_CURRENT_LIST_DIR}/type_db.cpp)