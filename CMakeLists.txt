cmake_minimum_required(VERSION 3.29)
project(entis)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/grl)

file(GLOB_RECURSE
        SRC_FILES
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

add_library(entis STATIC
        ${SRC_FILES}
        ${CMAKE_CURRENT_SOURCE_DIR}/include/entis/entis.h
)

target_include_directories(entis PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(entis PUBLIC grl)

add_executable(entis_benchmark benchmarks/benchmark.cpp)
target_link_libraries(entis_benchmark PRIVATE entis)

add_executable(entis_typeErasedBenchmark benchmarks/benchmarkTypeErased.cpp)
target_link_libraries(entis_typeErasedBenchmark PRIVATE entis)