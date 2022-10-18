
include(${CMAKE_CURRENT_LIST_DIR}/type_info/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/assets/CMakeLists.txt)

target_sources(${PROJECT_NAME} INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/arch.h
        ${CMAKE_CURRENT_LIST_DIR}/platform.h

        ${CMAKE_CURRENT_LIST_DIR}/alloc_util.hpp
        ${CMAKE_CURRENT_LIST_DIR}/table_util.hpp
        ${CMAKE_CURRENT_LIST_DIR}/aligned_storage.hpp
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
        ${CMAKE_CURRENT_LIST_DIR}/string_util.hpp

        ${CMAKE_CURRENT_LIST_DIR}/owned_ptr.hpp
        ${CMAKE_CURRENT_LIST_DIR}/flagged_integer.hpp)

target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp

        ${CMAKE_CURRENT_LIST_DIR}/assets.cpp
        ${CMAKE_CURRENT_LIST_DIR}/config.cpp
        ${CMAKE_CURRENT_LIST_DIR}/delegate_error.cpp
        ${CMAKE_CURRENT_LIST_DIR}/expected.cpp
        ${CMAKE_CURRENT_LIST_DIR}/intern.cpp
        ${CMAKE_CURRENT_LIST_DIR}/object.cpp
        ${CMAKE_CURRENT_LIST_DIR}/resources.cpp
        ${CMAKE_CURRENT_LIST_DIR}/service.cpp
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool.cpp
        ${CMAKE_CURRENT_LIST_DIR}/uuid.cpp
        ${CMAKE_CURRENT_LIST_DIR}/uri.cpp
        ${CMAKE_CURRENT_LIST_DIR}/zstd.cpp)
