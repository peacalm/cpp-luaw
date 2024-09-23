cmake_minimum_required(VERSION 3.14)

project(Luaw
        VERSION 1.0
        DESCRIPTION "Lua wrapper for C++"
        HOMEPAGE_URL "https://github.com/peacalm/cpp-luaw"
        LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
    # At least cpp14
    set(CMAKE_CXX_STANDARD 14)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED True)

SET(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include")
SET(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
SET(CMAKECONFIG_INSTALL_DIR "${LIB_INSTALL_DIR}/cmake/${PROJECT_NAME}")

find_package(Lua REQUIRED)

# BUILD_TEST could be: True/False constant like ON/OFF, or UNIT/PERF
option(BUILD_TEST "Build test." OFF)
option(NEED_VOLATILE "Support calling members by volatile objects." OFF)

if (NEED_VOLATILE)
    add_definitions(-DPEACALM_LUAW_NEED_VOLATILE)
endif()

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>)

target_link_libraries(${PROJECT_NAME} INTERFACE ${LUA_LIBRARIES})

if (BUILD_TEST)
    enable_testing()
    if ("${BUILD_TEST}" OR ("${BUILD_TEST}" STREQUAL "UNIT"))
        add_subdirectory(test/unit_test)
    endif()
    if ("${BUILD_TEST}" OR ("${BUILD_TEST}" STREQUAL "PERF"))
        add_subdirectory(test/perf_test)
    endif()
endif()

configure_file("${PROJECT_NAME}Config.cmake.in" "${PROJECT_NAME}Config.cmake"
        @ONLY)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/peacalm"
        DESTINATION ${INCLUDE_INSTALL_DIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        DESTINATION ${CMAKECONFIG_INSTALL_DIR})
