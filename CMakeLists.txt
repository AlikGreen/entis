cmake_minimum_required(VERSION 3.29)
project(neonECS)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE
        SRC_FILES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

add_library(neonECS STATIC
        ${SRC_FILES}
        ${CMAKE_CURRENT_SOURCE_DIR}/include/neonECS/neonECS.h
)

target_include_directories(neonECS PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(neonECS_test
        test/neonECSTest.cpp
)

target_link_libraries(neonECS_test PRIVATE neonECS)