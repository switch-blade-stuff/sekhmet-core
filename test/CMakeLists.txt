
add_executable(${SEK_CORE_PROJECT}-tests
        ${CMAKE_CURRENT_LIST_DIR}/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_events.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_dense_map.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_dense_set.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_dense_multiset.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_type_info.cpp)

target_link_libraries(${SEK_CORE_PROJECT}-tests PRIVATE ${SEK_CORE_PROJECT})

enable_testing()

macro(make_test)
    add_test(NAME ${ARGV0} COMMAND $<TARGET_FILE:${SEK_CORE_PROJECT}-tests> ${ARGV0})
endmacro()

make_test(events)
make_test(dense_map)
make_test(dense_set)
make_test(dense_multiset)
make_test(type_info)