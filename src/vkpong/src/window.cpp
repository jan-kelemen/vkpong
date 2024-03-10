#include <window.hpp>

// clang-format off
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

// clang-format on

vkpong::window::window(int width, int height)
    : impl_{nullptr, &glfwDestroyWindow}
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    impl_.reset(glfwCreateWindow(width, height, "vkpong", nullptr, nullptr));
}

void vkpong::window::loop(std::function<void()> callback)
{
    while (!glfwWindowShouldClose(impl_.get()))
    {
        glfwPollEvents();
        callback();
    }
}

vkpong::window::~window()
{
    impl_.reset();
    glfwTerminate();
}
