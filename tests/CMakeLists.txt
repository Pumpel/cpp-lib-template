cmake_minimum_required(VERSION 3.16)
project(Tests
        LANGUAGES CXX)

include(FetchContent)

option(INSTALL_GTEST OFF)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0
)
FetchContent_MakeAvailable(googletest)

add_executable(testBasicGTest testbasicgtest.cpp)
target_link_libraries(testBasicGTest PRIVATE gtest_main)
add_test(NAME testBasicGTest COMMAND testBasicGTest)

add_executable(testFooLib testFooLib.cpp)
target_link_libraries(testFooLib PRIVATE gtest_main FooLib)
add_test(NAME testFooLib COMMAND testFooLib)
