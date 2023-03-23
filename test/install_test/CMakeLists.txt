cmake_minimum_required(VERSION 3.14)

project(LuaWrapperInstallTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

enable_testing()

find_package(Lua REQUIRED)
if (NOT LUA_FOUND)
    message(FATAL_ERROR "Lua not found! Please install it first.")
endif()

find_package(LuaWrapper REQUIRED)
if (NOT LuaWrapper_FOUND)
    message(FATAL_ERROR "LuaWrapper not found! Please install it first.")
endif()

add_executable(install_test "install_test.cpp")

target_include_directories(install_test PUBLIC
        ${LUA_INCLUDE_DIR}
        ${LuaWrapper_INCLUDE_DIR})

target_link_libraries(install_test PUBLIC ${GTEST_LIBRARIES} ${LUA_LIBRARIES})

add_test(NAME install_test COMMAND install_test)

