cmake_minimum_required(VERSION 3.14)

project(LuaWrapperUnitTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

find_package(GTest REQUIRED)
if (NOT GTEST_FOUND)
    message(FATAL_ERROR "GTest not found! Please install it first.")
endif()

option(ENABLE_MYOSTREAM_WATCH "Use lib myostream to print variables to console." FALSE)
if (ENABLE_MYOSTREAM_WATCH)
    find_package(MyOStream REQUIRED)
    if (NOT MyOStream_FOUND)
        message(FATAL_ERROR "MyOStream not found! Please install it first.")
    endif()
    add_definitions(-DENABLE_MYOSTREAM_WATCH)
endif()

find_package(Lua REQUIRED)
if (NOT LUA_FOUND)
    message(FATAL_ERROR "Lua not found! Please install it first.")
endif()

file(GLOB SOURCE_FILES "unit_test.cpp")
SET(TARGET "unit_test")

add_executable(${TARGET} ${SOURCE_FILES})

target_include_directories(${TARGET} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${LUA_INCLUDE_DIR}
        ${MyOStream_INCLUDE_DIR})

target_link_libraries(${TARGET} PUBLIC ${GTEST_LIBRARIES} ${LUA_LIBRARIES})

add_test(NAME ${TARGET} COMMAND ${TARGET})

include(GoogleTest)
gtest_discover_tests(${TARGET})