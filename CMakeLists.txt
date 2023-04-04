cmake_minimum_required(VERSION 3.14)

project(cpp_toolkit)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

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
    unittest/object_pool/test.cpp
    unittest/thread_pool/test.cpp
    unittest/concurrent_lru_cache/test.cpp
    unittest/config_parser/test.cpp
    unittest/mem_pool/test.cpp
)

IF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(cpp_toolkit_test GTest::gtest_main -fno-omit-frame-pointer)
    target_compile_options(cpp_toolkit_test PRIVATE -fno-omit-frame-pointer) 
ELSE ()
    target_link_libraries(cpp_toolkit_test GTest::gtest_main -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer)
    target_compile_options(cpp_toolkit_test PRIVATE -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -lpthread)
ENDIF()

include(GoogleTest)
gtest_discover_tests(cpp_toolkit_test)