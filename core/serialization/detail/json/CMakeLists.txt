
list(APPEND SEK_CORE_INTERFACE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/json_error.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type.hpp
        ${CMAKE_CURRENT_LIST_DIR}/object.hpp
        ${CMAKE_CURRENT_LIST_DIR}/json_reader.hpp
        ${CMAKE_CURRENT_LIST_DIR}/json_writer.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ubj_reader.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ubj_writer.hpp)

list(APPEND SEK_CORE_PRIVATE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/json_error.cpp
        ${CMAKE_CURRENT_LIST_DIR}/object.cpp)