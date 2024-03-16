#ifndef VKPONG_VULKAN_CONTEXT_INCLUDED
#define VKPONG_VULKAN_CONTEXT_INCLUDED

#include <vulkan/vulkan_core.h>

#include <optional>

struct GLFWwindow;

namespace vkpong
{
    class [[nodiscard]] vulkan_context final
    {
    public: // Construction
        vulkan_context(VkInstance instance_,
            std::optional<VkDebugUtilsMessengerEXT> debug_messenger,
            VkSurfaceKHR surface);

        vulkan_context(vulkan_context const&) = delete;

        vulkan_context(vulkan_context&& other) noexcept;

    public: // Destruction
        ~vulkan_context();

    public: // Interface
        [[nodiscard]] constexpr VkInstance instance() const noexcept;

        [[nodiscard]] constexpr VkSurfaceKHR surface() const noexcept;

    public: // Operators
        vulkan_context& operator=(vulkan_context const&) = delete;

        vulkan_context& operator=(vulkan_context&& other) noexcept;

    private: // Data
        VkInstance instance_;
        std::optional<VkDebugUtilsMessengerEXT> debug_messenger_;
        VkSurfaceKHR surface_;
    };

    vulkan_context create_context(GLFWwindow* window,
        bool setup_validation_layers);
} // namespace vkpong

inline vkpong::vulkan_context::vulkan_context(VkInstance instance,
    std::optional<VkDebugUtilsMessengerEXT> debug_messenger,
    VkSurfaceKHR surface)
    : instance_{instance}
    , debug_messenger_{debug_messenger}
    , surface_{surface}
{
}

inline vkpong::vulkan_context::vulkan_context(vulkan_context&& other) noexcept
    : instance_{std::exchange(other.instance_, nullptr)}
    , debug_messenger_{std::exchange(other.debug_messenger_, {})}
    , surface_{std::exchange(other.surface_, nullptr)}
{
}

inline constexpr VkInstance vkpong::vulkan_context::instance() const noexcept
{
    return instance_;
}

inline constexpr VkSurfaceKHR vkpong::vulkan_context::surface() const noexcept
{
    return surface_;
}

inline vkpong::vulkan_context& vkpong::vulkan_context::operator=(
    vulkan_context&& other) noexcept
{
    using std::swap;

    if (this != &other)
    {
        swap(instance_, other.instance_);
        swap(debug_messenger_, other.debug_messenger_);
        swap(surface_, other.surface_);
    }

    return *this;
}

#endif // !VKPONG_VULKAN_CONTEXT_INCLUDED
