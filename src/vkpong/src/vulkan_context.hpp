#ifndef VKPONG_VULKAN_CONTEXT_INCLUDED
#define VKPONG_VULKAN_CONTEXT_INCLUDED

#include <vulkan/vulkan_core.h>

#include <memory>
#include <optional>

struct GLFWwindow;

namespace vkpong
{
    class [[nodiscard]] vulkan_context final
    {
    public:
        VkInstance instance{};
        std::optional<VkDebugUtilsMessengerEXT> debug_messenger{};
        VkSurfaceKHR surface{};

    public: // Destruction
        ~vulkan_context();
    };

    std::unique_ptr<vulkan_context> create_context(GLFWwindow* window,
        bool setup_validation_layers);
} // namespace vkpong

#endif // !VKPONG_VULKAN_CONTEXT_INCLUDED
