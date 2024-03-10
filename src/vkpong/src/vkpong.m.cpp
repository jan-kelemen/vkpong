#include <vulkan_context.hpp>
#include <vulkan_device.hpp>
#include <vulkan_pipeline.hpp>
#include <vulkan_renderer.hpp>
#include <vulkan_swap_chain.hpp>
#include <window.hpp>

#ifdef NDEBUG
constexpr bool enable_validation_layers{false};
#else
constexpr bool enable_validation_layers{true};
#endif

int main()
{
    vkpong::window window;
    {
        auto context{
            vkpong::create_context(window.handle(), enable_validation_layers)};
        auto device{vkpong::create_device(context.get())};
        auto swap_chain{
            std::make_unique<vkpong::vulkan_swap_chain>(window.handle(),
                context.get(),
                device.get())};
        auto pipeline{vkpong::create_pipeline(device.get(), swap_chain.get())};
        auto renderer{vkpong::vulkan_renderer{std::move(context),
            std::move(device),
            std::move(swap_chain),
            std::move(pipeline)}};

        window.loop([&renderer]() { renderer.draw(); });
    }
}
