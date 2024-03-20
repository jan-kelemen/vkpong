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
#ifdef NDEBUG
    constexpr bool enable_validation_layers{false};
#else
    constexpr bool enable_validation_layers{true};
#endif

    class [[nodiscard]] vkpong_app final
    {
    public: // Construction
        vkpong_app(int width, int height)
            : window_{width, height}
            , context_{vkpong::create_context(window_.handle(),
                  enable_validation_layers)}
            , device_{vkpong::create_device(context_)}
            , swap_chain_{window_.handle(), &context_, &device_}
            , renderer_{&context_, &device_, &swap_chain_}
        {
            glfwSetWindowUserPointer(window_.handle(), this);
            glfwSetFramebufferSizeCallback(window_.handle(),
                framebuffer_resize_callback);
            glfwSetKeyCallback(window_.handle(), key_callback);
        }

        vkpong_app(vkpong_app const&) = delete;

        vkpong_app(vkpong_app&&) noexcept = delete;

    public: // Destruction
        ~vkpong_app() = default;

    public: // Interface
        void run()
        {
            window_.loop([this]() { renderer_.draw(game_); });
        }

    public: // Operators
        vkpong_app& operator=(vkpong_app const&) = delete;

        vkpong_app& operator=(vkpong_app&&) noexcept = delete;

    private: // Helpers
        static void key_callback(GLFWwindow* window,
            int key,
            [[maybe_unused]] int scancode,
            int action,
            [[maybe_unused]] int mods)
        {
            // NOLINTNEXTLINE
            auto* const app{reinterpret_cast<vkpong_app*>(
                glfwGetWindowUserPointer(window))};

            if (action == GLFW_PRESS || action == GLFW_REPEAT)
            {
                if (key == GLFW_KEY_UP)
                {
                    app->action(vkpong::action::up);
                }
                else if (key == GLFW_KEY_DOWN)
                {
                    app->action(vkpong::action::down);
                }
            }
        }

        static void framebuffer_resize_callback(GLFWwindow* window,
            [[maybe_unused]] int width,
            [[maybe_unused]] int height)
        {
            // NOLINTNEXTLINE
            auto* const app{reinterpret_cast<vkpong_app*>(
                glfwGetWindowUserPointer(window))};
            app->resized();
        }

        void resized() { swap_chain_.resized(); }

        void action(vkpong::action act) { game_.update(act); }

    private: // Data
        vkpong::game game_;
        vkpong::window window_;
        vkpong::vulkan_context context_;
        vkpong::vulkan_device device_;
        vkpong::vulkan_swap_chain swap_chain_;
        vkpong::vulkan_renderer renderer_;
    };
} // namespace

int main()
{
    try
    {
        vkpong_app app{vkpong::window::default_width,
            vkpong::window::default_height};
        app.run();
    }
    catch (std::exception const& ex)
    {
        spdlog::error("Uncaught exception: {}", ex.what());
        throw;
    }
}
