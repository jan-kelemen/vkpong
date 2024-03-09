add_executable(vkpong)

target_sources(vkpong
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vkpong.m.cpp
)

target_link_libraries(vkpong
    PRIVATE
        glfw
        glm::glm
        vulkan-headers::vulkan-headers
        Vulkan::Loader
        project-options
)
