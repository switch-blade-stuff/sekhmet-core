
include(${CMAKE_CURRENT_LIST_DIR}/detail/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/system/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/math/CMakeLists.txt)

# TODO: Uncomment when Json rework is complete
#include(${CMAKE_CURRENT_LIST_DIR}/serialization/CMakeLists.txt)

include(${CMAKE_CURRENT_LIST_DIR}/rhi/CMakeLists.txt)

list(APPEND SEK_CORE_INTERFACE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/define.h
        ${CMAKE_CURRENT_LIST_DIR}/assert.hpp

        ${CMAKE_CURRENT_LIST_DIR}/meta.hpp
        ${CMAKE_CURRENT_LIST_DIR}/hash.hpp
        ${CMAKE_CURRENT_LIST_DIR}/access_guard.hpp
        ${CMAKE_CURRENT_LIST_DIR}/expected.hpp
        ${CMAKE_CURRENT_LIST_DIR}/delegate.hpp
        ${CMAKE_CURRENT_LIST_DIR}/event.hpp
        ${CMAKE_CURRENT_LIST_DIR}/property.hpp
        ${CMAKE_CURRENT_LIST_DIR}/thread_pool.hpp
        ${CMAKE_CURRENT_LIST_DIR}/static_string.hpp
        ${CMAKE_CURRENT_LIST_DIR}/uri.hpp
        ${CMAKE_CURRENT_LIST_DIR}/uuid.hpp
        ${CMAKE_CURRENT_LIST_DIR}/version.hpp

        ${CMAKE_CURRENT_LIST_DIR}/buffered_vector.hpp
        ${CMAKE_CURRENT_LIST_DIR}/interned_string.hpp
        ${CMAKE_CURRENT_LIST_DIR}/dense_map.hpp
        ${CMAKE_CURRENT_LIST_DIR}/dense_set.hpp
        ${CMAKE_CURRENT_LIST_DIR}/dense_multiset.hpp
        ${CMAKE_CURRENT_LIST_DIR}/sparse_map.hpp
        ${CMAKE_CURRENT_LIST_DIR}/sparse_set.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ordered_map.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ordered_set.hpp

        ${CMAKE_CURRENT_LIST_DIR}/logger.hpp
        ${CMAKE_CURRENT_LIST_DIR}/plugin.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_name.hpp
        ${CMAKE_CURRENT_LIST_DIR}/type_info.hpp
        ${CMAKE_CURRENT_LIST_DIR}/object.hpp
        ${CMAKE_CURRENT_LIST_DIR}/service.hpp

        ${CMAKE_CURRENT_LIST_DIR}/math.hpp
        ${CMAKE_CURRENT_LIST_DIR}/system.hpp
        ${CMAKE_CURRENT_LIST_DIR}/utility.hpp)

