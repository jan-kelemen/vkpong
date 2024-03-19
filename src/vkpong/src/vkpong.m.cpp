#include <game.hpp>
#include <vulkan_context.hpp>
#include <vulkan_device.hpp>
#include <vulkan_renderer.hpp>
#include <vulkan_swap_chain.hpp>
#include <window.hpp>

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <exception>
#include <functional>
#include <memory>
#include <utility>

namespace
{
    vkpong::game state;

#ifdef NDEBUG
    constexpr bool enable_validation_layers{false};
#else
    constexpr bool enable_validation_layers{true};
#endif

    void key_callback([[maybe_unused]] GLFWwindow* window,
        int key,
        [[maybe_unused]] int scancode,
        int action,
        [[maybe_unused]] int mods)
    {
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            if (key == GLFW_KEY_UP)
            {
                state.update(vkpong::action::up);
            }
            else if (key == GLFW_KEY_DOWN)
            {
                state.update(vkpong::action::down);
            }
        }
    }

} // namespace

int main()
{
    try
    {
        vkpong::window window;
        glfwSetKeyCallback(window.handle(), key_callback);

        {
            auto context{std::make_unique<vkpong::vulkan_context>(
                vkpong::create_context(window.handle(),
                    enable_validation_layers))};
            auto device{std::make_unique<vkpong::vulkan_device>(
                vkpong::create_device(*context))};
            auto swap_chain{
                std::make_unique<vkpong::vulkan_swap_chain>(window.handle(),
                    context.get(),
                    device.get())};
            auto renderer{vkpong::vulkan_renderer{std::move(context),
                std::move(device),
                std::move(swap_chain)}};

            window.loop([&renderer]() { renderer.draw(state); });
        }
    }
    catch (std::exception const& ex)
    {
        spdlog::error("Uncaught exception: {}", ex.what());
        throw;
    }
}
