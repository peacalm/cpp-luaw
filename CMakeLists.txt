cmake_minimum_required(VERSION 3.14)

project(Luaw
        VERSION 1.0
        DESCRIPTION "Lua wrapper for C++"
        HOMEPAGE_URL "https://github.com/peacalm/cpp-luaw"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

SET(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")
SET(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKECONFIG_INSTALL_DIR "${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME}")

find_package(Lua REQUIRED)

option(BUILD_TEST "Build test." FALSE)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>)

target_link_libraries(${PROJECT_NAME} INTERFACE ${LUA_LIBRARIES})

if (BUILD_TEST)
    enable_testing()
    add_subdirectory(test/unit_test)
    add_subdirectory(test/perf_test)
endif()

configure_file("${PROJECT_NAME}Config.cmake.in" "${PROJECT_NAME}Config.cmake"
        @ONLY)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/peacalm"
        DESTINATION ${INCLUDE_INSTALL_DIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})
