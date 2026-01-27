cmake_minimum_required(VERSION 3.29)
project(neonECS)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/neonCore)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/unordered_dense)

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

target_link_libraries(neonECS PUBLIC neonCore unordered_dense)

add_executable(neonECS_benchmark benchmarks/benchmark.cpp)
target_link_libraries(neonECS_benchmark PRIVATE neonECS)

add_executable(neonECS_typeErasedBenchmark benchmarks/benchmarkTypeErased.cpp)
target_link_libraries(neonECS_typeErasedBenchmark PRIVATE neonECS)