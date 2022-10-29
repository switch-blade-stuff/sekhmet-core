
add_executable(${SEK_CORE_PROJECT}-tests
        ${CMAKE_CURRENT_LIST_DIR}/main.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_dense_map.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_dense_set.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test_dense_multiset.cpp)

target_link_libraries(${SEK_CORE_PROJECT}-tests PRIVATE ${SEK_CORE_PROJECT})

enable_testing()

add_test(NAME dense_map COMMAND $<TARGET_FILE:${SEK_CORE_PROJECT}-tests> dense_map)
add_test(NAME dense_set COMMAND $<TARGET_FILE:${SEK_CORE_PROJECT}-tests> dense_set)
#add_test(NAME dense_multiset COMMAND $<TARGET_FILE:${SEK_CORE_PROJECT}-tests> dense_multiset)