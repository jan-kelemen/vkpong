add_executable(vkpong)

target_sources(vkpong
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vkpong.m.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_context.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_context.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_device.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_device.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_swap_chain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vulkan_swap_chain.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/window.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/window.hpp
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
        project-options
)
