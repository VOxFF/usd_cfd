cmake_minimum_required(VERSION 3.20)
project(usd_cfd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(OpenVDB REQUIRED)

# Pull in usd_fluid_domain as a submodule (calls find_package(pxr) internally;
# tests disabled to avoid pulling in GoogleTest twice)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/usd_fluid_domain)

option(USD_CFD_BUILD_TESTS "Build usd_cfd unit tests" ON)
set(BUILD_TESTS ${USD_CFD_BUILD_TESTS} CACHE BOOL "" FORCE)

add_subdirectory(src)
add_subdirectory(include)

add_executable(usd_cfd main.cpp)
target_link_libraries(usd_cfd PRIVATE ufc)

if(USD_CFD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
