cmake_minimum_required(VERSION 3.14)

project(LuawUnitTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

find_package(Lua REQUIRED)
find_package(GTest REQUIRED)

option(ENABLE_MYOSTREAM_WATCH "Use lib myostream to print variables to console." OFF)
option(UNIT_TEST_SEPARATE "Whether build unit tests separately." OFF)
option(UTONLY "Only build one unit test file." "")

if (ENABLE_MYOSTREAM_WATCH)
    find_package(MyOStream REQUIRED)
    add_definitions(-DENABLE_MYOSTREAM_WATCH)
endif()

function(build_target TARGET)
    target_include_directories(${TARGET} PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/
            ${CMAKE_CURRENT_SOURCE_DIR}/../../include
            ${LUA_INCLUDE_DIR}
            ${MyOStream_INCLUDE_DIR})
    target_link_libraries(${TARGET} PUBLIC ${GTEST_LIBRARIES} ${LUA_LIBRARIES})
    # add_test(NAME ${TARGET} COMMAND ${TARGET})
    include(GoogleTest)
    gtest_discover_tests(${TARGET})
endfunction()

file(GLOB SOURCE_FILES "*.cpp")

if (UNIT_TEST_SEPARATE)
    foreach(FILE ${SOURCE_FILES})
        get_filename_component(TARGET ${FILE} NAME_WE)
        if (NOT ${UTONLY} STREQUAL "" AND NOT ${TARGET} STREQUAL ${UTONLY})
            continue()
        endif()
        add_executable(${TARGET} ${FILE} "main.cpp")
        build_target(${TARGET})
    endforeach()
else()
    SET(TARGET "unit_test")
    add_executable(${TARGET} ${SOURCE_FILES})
    build_target(${TARGET})
endif()
