cmake_minimum_required(VERSION 3.30)
project(refactor_tool VERSION 1.0.0 LANGUAGES CXX)

#----------------------------------------------------------------------------------------------------------------------
# testing framework
#----------------------------------------------------------------------------------------------------------------------

include(FetchContent)
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # do not override parent project's runtime settings on Windows
set(INSTALL_GTEST OFF)

# For simplicity, always build googletest as static library. This prevents mylib-tests executable from
# complaining about missing googletest DLLs on Windows.
set(BUILD_SHARED_LIBS OFF)

FetchContent_MakeAvailable(googletest)

#
# Тесты
#

# Рекурсивно ищем все используемые в тестах .cpp файлы
file(GLOB TEST_SRC_FILES "${CMAKE_SOURCE_DIR}/tests/*.cpp")

add_executable(${PROJECT_NAME}_tests
        "${CMAKE_SOURCE_DIR}/src/RefactorTool.cpp"
        "${TEST_SRC_FILES}"
)

# Force C++17 standard (same as LLVM)
set_target_properties(${PROJECT_NAME}_tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

#target_link_libraries(${PROJECT_NAME}_tests PRIVATE gtest_main)
#TODO: didn't compile without these
target_link_libraries(
        refactor_tool_tests
        PRIVATE
        gmock
        gmock_main
        GTest::gtest
        GTest::gtest_main
        # Clang libraries
        clangTooling
        clangFrontend
        clangASTMatchers
        clangAST
        clangBasic
        clangLex
        clangRewrite
        clangSerialization
        clangDriver
        clangParse
        clangSema
)

target_include_directories(${PROJECT_NAME}_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
)

include(GoogleTest)
gtest_discover_tests(refactor_tool_tests)