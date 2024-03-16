add_executable(vkpong)

target_sources(vkpong
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scope_exit.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vkpong.m.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_buffer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_buffer.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_context.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_device.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_pipeline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_pipeline.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_renderer.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_swap_chain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_swap_chain.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_utility.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_utility.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/window.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/window.hpp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert
        ${CMAKE_CURRENT_BINARY_DIR}/vert.spv
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag
        ${CMAKE_CURRENT_BINARY_DIR}/frag.spv
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vert.spv
    COMMAND 
        ${GLSLC_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert -o ${CMAKE_CURRENT_BINARY_DIR}/vert.spv
    DEPENDS 
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/frag.spv
    COMMAND 
        ${GLSLC_EXE} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag -o ${CMAKE_CURRENT_BINARY_DIR}/frag.spv
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag
)

target_include_directories(vkpong
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(vkpong
    PRIVATE
        glfw
        glm::glm
        vulkan-headers::vulkan-headers
        Vulkan::Loader
        spdlog::spdlog
        project-options
)

source_group("Header Files"
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scope_exit.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_buffer.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_context.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_device.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_pipeline.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_renderer.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_swap_chain.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_utility.hpp
)
source_group("Source Files"
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vkpong.m.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_buffer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_pipeline.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_renderer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_swap_chain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_utility.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/window.cpp
)

source_group("Shader Files"
    FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.frag
)

set_property(TARGET vkpong 
    PROPERTY 
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
