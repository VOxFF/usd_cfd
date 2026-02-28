cmake_minimum_required(VERSION 3.20)
project(usd_cfd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(pxr REQUIRED
    HINTS $ENV{USD_ROOT} ${USD_ROOT}
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(OpenVDB REQUIRED)

add_subdirectory(src)
add_subdirectory(include)

add_executable(usd_cfd main.cpp)
target_link_libraries(usd_cfd PRIVATE ufc)

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
