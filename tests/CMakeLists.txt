include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

FetchContent_MakeAvailable(googletest)

add_executable(usd_cfd_tests)
add_subdirectory(src)

target_compile_definitions(usd_cfd_tests PRIVATE
    TEST_RESOURCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
)

target_link_libraries(usd_cfd_tests
    PRIVATE ufc
    PRIVATE GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(usd_cfd_tests)
