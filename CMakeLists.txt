cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(osusat-core
    VERSION 1.0.0
    DESCRIPTION "OSUSat Embedded Systems Primitives Library"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

cmake_policy(SET CMP0135 NEW)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_COMPILER_IS_GNUCC)
    add_compile_options(-Wall -Wextra -Wpedantic)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    endif()
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    endif()
endif()

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_program(CLANG_FORMAT_EXECUTABLE clang-format)
find_program(CLANG_TIDY_EXECUTABLE clang-tidy)

file(GLOB_RECURSE C_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

file(GLOB_RECURSE C_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/osusat/*.h"
)

if(C_SOURCES)
    add_library(osusat-core STATIC ${C_SOURCES})
    target_include_directories(osusat-core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
else()
    add_library(osusat-core INTERFACE)
    target_include_directories(osusat-core INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()

add_library(OSUSat::Core ALIAS osusat-core)

if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(core_format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${C_SOURCES} ${C_HEADERS}
        COMMENT "Formatting OSUSat embedded primitives"
        VERBATIM
    )
endif()

if(CLANG_TIDY_EXECUTABLE)
    add_custom_target(core_lint
        COMMAND ${CLANG_TIDY_EXECUTABLE} ${C_SOURCES} --
            -I${CMAKE_CURRENT_SOURCE_DIR}/include
        COMMENT "Linting OSUSat embedded primitives"
        VERBATIM
    )
endif()

message(STATUS "=== OSUSat Embedded Systems Library ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Standard: ${CMAKE_C_STANDARD}")
if(CLANG_FORMAT_EXECUTABLE)
    message(STATUS "clang-format: ${CLANG_FORMAT_EXECUTABLE}")
endif()
if(CLANG_TIDY_EXECUTABLE)
    message(STATUS "clang-tidy: ${CLANG_TIDY_EXECUTABLE}")
endif()
message(STATUS "========================================")

install(TARGETS osusat-core
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/osusat/ DESTINATION include/osusat
    FILES_MATCHING PATTERN "*.h"
)

find_package(Doxygen REQUIRED COMPONENTS doxygen)
find_program(SPHINX_BUILD sphinx-build)

if (DOXYGEN_FOUND)
    set(DOXYFILE ${CMAKE_CURRENT_SOURCE_DIR}/documentation/doxygen/Doxyfile)

    add_custom_target(core_doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/documentation/doxygen
        COMMENT "Generating Doxygen XML API documentation"
        VERBATIM)

    if (SPHINX_BUILD)
        add_custom_target(core_sphinx
            COMMAND ${SPHINX_BUILD}
                    -b html
                    ${CMAKE_CURRENT_SOURCE_DIR}/documentation/sphinx/source
                    ${CMAKE_CURRENT_BINARY_DIR}/documentation/sphinx
            COMMENT "Generating Sphinx documentation"
            VERBATIM)

        add_dependencies(core_sphinx core_doxygen)

        add_custom_target(core_docs
            DEPENDS core_doxygen core_sphinx)
    endif()
endif()

option(BUILD_TESTING "Build the testing tree" ON)

if(BUILD_TESTING)
    enable_testing()

    # discover all test files
    file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.c")

    foreach(TEST_SOURCE ${TEST_SOURCES})
        # extract test name from filename (e.g., test_ring_buffer.c -> test_ring_buffer)
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

        add_executable(${TEST_NAME} ${TEST_SOURCE})

        target_link_libraries(${TEST_NAME} PRIVATE osusat-core)

        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

        set_tests_properties(${TEST_NAME} PROPERTIES
            TIMEOUT 10
            LABELS "unit"
        )
    endforeach()

    message(STATUS "testing enabled - found ${CMAKE_MATCH_COUNT} test files")

    add_custom_target(test_verbose
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure
        DEPENDS ${TEST_SOURCES}
        COMMENT "Running all tests with verbose output"
    )
endif()
