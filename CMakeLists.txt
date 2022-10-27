cmake_minimum_required(VERSION 3.14)

project(cpp_toolkit)

set(CMAKE_CXX_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include_directories(
    ${GTEST_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR})

enable_testing()

add_executable(
    cpp_toolkit_test
    unittest/objectpool/test.cpp
    unittest/threadpool/test.cpp)

target_link_libraries(cpp_toolkit_test GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(cpp_toolkit_test)