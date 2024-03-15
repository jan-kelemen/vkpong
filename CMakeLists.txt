cmake_minimum_required(VERSION 3.27)

project(vkpong VERSION 0.1 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(VKPONG_ENABLE_CLANG_FORMAT "Enable clang-format in build" OFF)
option(VKPONG_ENABLE_CLANG_TIDY "Enable clang-tidy in build" OFF)
option(VKPONG_ENABLE_COMPILER_STATIC_ANALYSIS "Enable static analysis provided by compiler in build" OFF)
option(VKPONG_ENABLE_CPPCHECK "Enable cppcheck in build" OFF)
option(VKPONG_ENABLE_IWYU "Enable include-what-you-use in build" OFF)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)
find_package(VulkanHeaders REQUIRED)
find_package(VulkanLoader REQUIRED)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(${PROJECT_SOURCE_DIR}/cmake/clang-tidy.cmake)
    include(${PROJECT_SOURCE_DIR}/cmake/cppcheck.cmake)
    include(${PROJECT_SOURCE_DIR}/cmake/iwyu.cmake)

    include(${PROJECT_SOURCE_DIR}/cmake/project-options.cmake)
endif()

include(${PROJECT_SOURCE_DIR}/cmake/glslc.cmake)

add_subdirectory(src)
