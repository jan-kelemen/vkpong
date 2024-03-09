#include "window.hpp"

// clang-format off
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
// clang-format on

#include <iostream>

namespace
{
    static void framebuffer_resize_callback(
        [[maybe_unused]] GLFWwindow* const window,
        int const width,
        int const height)
    {
        // TODO-JK log this properly and forward the event
        std::cerr << "Window resized to " << width << 'x' << height << '\n';
    }
} // namespace

vkpong::window::window(int width, int height)
    : impl_{nullptr, &glfwDestroyWindow}
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    impl_.reset(glfwCreateWindow(width, height, "vkpong", nullptr, nullptr));
    glfwSetFramebufferSizeCallback(impl_.get(), framebuffer_resize_callback);
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
