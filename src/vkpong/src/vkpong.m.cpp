#include <vulkan_context.hpp>
#include <vulkan_device.hpp>
#include <vulkan_renderer.hpp>
#include <vulkan_swap_chain.hpp>
#include <window.hpp>

#include <spdlog/spdlog.h>

#ifdef NDEBUG
constexpr bool enable_validation_layers{false};
#else
constexpr bool enable_validation_layers{true};
#endif

int main()
{
    try
    {
        vkpong::window window;
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

            window.loop([&renderer]() { renderer.draw(); });
        }
    }
    catch (std::exception const& ex)
    {
        spdlog::error("Uncaught exception: {}", ex.what());
        throw;
    }
}
