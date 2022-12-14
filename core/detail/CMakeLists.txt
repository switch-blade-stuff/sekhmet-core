
include(${CMAKE_CURRENT_LIST_DIR}/type_info/CMakeLists.txt)

list(APPEND SEK_CORE_INTERFACE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/arch.h
        ${CMAKE_CURRENT_LIST_DIR}/platform.h

        ${CMAKE_CURRENT_LIST_DIR}/alloc_util.hpp
        ${CMAKE_CURRENT_LIST_DIR}/table_util.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_storage.hpp
        ${CMAKE_CURRENT_LIST_DIR}/basic_pool.hpp
        ${CMAKE_CURRENT_LIST_DIR}/buffer_allocator.hpp
        ${CMAKE_CURRENT_LIST_DIR}/contiguous_iterator.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ebo_base_helper.hpp
        ${CMAKE_CURRENT_LIST_DIR}/dense_hash_table.hpp
        ${CMAKE_CURRENT_LIST_DIR}/sparse_hash_table.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ordered_hash_table.hpp
        ${CMAKE_CURRENT_LIST_DIR}/packed_pair.hpp

        ${CMAKE_CURRENT_LIST_DIR}/event.hpp
        ${CMAKE_CURRENT_LIST_DIR}/event_proxy.hpp

        ${CMAKE_CURRENT_LIST_DIR}/owned_ptr.hpp
        ${CMAKE_CURRENT_LIST_DIR}/flagged_integer.hpp)

list(APPEND SEK_CORE_PRIVATE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/assert.cpp
        ${CMAKE_CURRENT_LIST_DIR}/expected.cpp
        ${CMAKE_CURRENT_LIST_DIR}/delegate_error.cpp
        ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
        ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
        ${CMAKE_CURRENT_LIST_DIR}/service.cpp

        ${CMAKE_CURRENT_LIST_DIR}/intern.cpp
        ${CMAKE_CURRENT_LIST_DIR}/object.cpp
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool.cpp
        ${CMAKE_CURRENT_LIST_DIR}/uuid.cpp
        ${CMAKE_CURRENT_LIST_DIR}/uri.cpp)
