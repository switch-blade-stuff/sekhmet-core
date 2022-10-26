cmake_minimum_required(VERSION 3.20)
project(sekhmet-core)

# Handle pre C++20 ABI-freeze MSVC versions
if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL 19.30)
    set(CMAKE_CXX_STANDARD 23) # /std:c++latest required for MSVC pre abi-freeze
else ()
    set(CMAKE_CXX_STANDARD 20)
endif ()

# Project build options
option(SEK_ENABLE_IPO "Enable IPO support" ON)
option(SEK_ENABLE_TESTS "Build unit tests" OFF)
option(SEK_BUILD_SHARED "Build shared library" ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(SEK_ENABLE_TESTS ON CACHE BOOL "Build unit tests" FORCE)
endif ()

# Project dependencies
find_package(Threads REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)

# Initialization macros
macro(sek_set_build_options)
    if (MSVC)
        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            target_compile_options(${ARGV0} PUBLIC /W4 /wd4251)
        else ()
            target_compile_options(${ARGV0} PUBLIC /W3)
        endif ()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # Enable all warnings & errors for the current project & children
        target_compile_options(${ARGV0} PUBLIC "-Wall" "-Wextra" "-Wno-unknown-pragmas" "-Wconversion" "-Werror")

        if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
            target_compile_options(${ARGV0} PUBLIC "-fconcepts-diagnostics-depth=32")
        else ()
            # Always set file prefix map to current directory to avoid exposing full directory tree in debug info
            target_compile_options(${ARGV0} PUBLIC "-ffile-prefix-map=\"${CMAKE_CURRENT_LIST_DIR}/\"=\"\"")
        endif ()
    endif ()

    if (SEK_ENABLE_IPO)
        include(CheckIPOSupported OPTIONAL RESULT_VARIABLE IPO_MODULE)
        if (NOT IPO_MODULE STREQUAL "NOTFOUND")
            check_ipo_supported(RESULT IPO_FOUND OUTPUT IPO_ERR)
            if (IPO_FOUND)
                set_property(TARGET ${ARGV0} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
            else ()
                message(WARNING "IPO is not supported by the toolchain: ${IPO_ERR}")
            endif ()
        else ()
            message(WARNING "Failed to include IPO CMake module")
        endif ()
    endif ()
endmacro()

macro(sek_set_visibility)
    set_target_properties(${ARGV0} PROPERTIES C_VISIBILITY_PRESET hidden)
    set_target_properties(${ARGV0} PROPERTIES CXX_VISIBILITY_PRESET hidden)
    set_target_properties(${ARGV0} PROPERTIES VISIBILITY_INLINES_HIDDEN ON)
    set_target_properties(${ARGV0} PROPERTIES DEFINE_SYMBOL "SEK_CORE_EXPORT")
endmacro()

# Set project-related variables & properties
set(SEK_CORE_PROJECT ${PROJECT_NAME})
set(SEK_CORE_SHARED_LIBRARY_PATH $<TARGET_FILE:${SEK_CORE_PROJECT}-shared>)
set(SEK_CORE_STATIC_LIBRARY_PATH $<TARGET_FILE:${SEK_CORE_PROJECT}-static>)
set(SEK_CORE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/core)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib CACHE STRING "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "")

# Add project targets & include directories
add_library(${SEK_CORE_PROJECT}-static STATIC)
add_library(${SEK_CORE_PROJECT}-shared SHARED)
if (SEK_BUILD_SHARED)
    add_library(${SEK_CORE_PROJECT} ALIAS ${SEK_CORE_PROJECT}-shared)
else ()
    add_library(${SEK_CORE_PROJECT} ALIAS ${SEK_CORE_PROJECT}-static)
endif ()
add_library(${SEK_CORE_PROJECT}-interface INTERFACE)

include_directories(${FMT_INCLUDE_DIR})
include_directories(${ASIO_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR})

# Set symbol visibility
sek_set_visibility(${SEK_CORE_PROJECT}-static)
sek_set_visibility(${SEK_CORE_PROJECT}-shared)

# Set compiler options
sek_set_build_options(${SEK_CORE_PROJECT}-static)
sek_set_build_options(${SEK_CORE_PROJECT}-shared)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include source tree & add sources
include(${SEK_CORE_SOURCE_DIR}/CMakeLists.txt)
target_sources(${SEK_CORE_PROJECT}-interface INTERFACE ${SEK_CORE_INTERFACE_SOURCES})
target_sources(${SEK_CORE_PROJECT}-static INTERFACE ${SEK_CORE_INTERFACE_SOURCES})
target_sources(${SEK_CORE_PROJECT}-shared INTERFACE ${SEK_CORE_INTERFACE_SOURCES})
target_sources(${SEK_CORE_PROJECT}-static PRIVATE ${SEK_CORE_PRIVATE_SOURCES})
target_sources(${SEK_CORE_PROJECT}-shared PRIVATE ${SEK_CORE_PRIVATE_SOURCES})
target_sources(${SEK_CORE_PROJECT}-static PUBLIC ${SEK_CORE_PUBLIC_SOURCES})
target_sources(${SEK_CORE_PROJECT}-shared PUBLIC ${SEK_CORE_PUBLIC_SOURCES})

# Link dependencies
target_link_libraries(${SEK_CORE_PROJECT}-static PUBLIC fmt::fmt asio::asio ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${SEK_CORE_PROJECT}-shared PUBLIC fmt::fmt asio::asio ${CMAKE_THREAD_LIBS_INIT})

# Add unit tests
if (SEK_ENABLE_TESTS)
    include(${CMAKE_CURRENT_LIST_DIR}/test/CMakeLists.txt)
endif ()
message("Core install prefix: ${CMAKE_INSTALL_PREFIX}")