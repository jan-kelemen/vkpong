#ifndef VKPONG_VULKAN_DEVICE_INCLUDED
#define VKPONG_VULKAN_DEVICE_INCLUDED

#include <vulkan/vulkan_core.h>

#include <memory>

namespace vkpong
{
    class vulkan_context;
} // namespace vkpong

namespace vkpong
{
    class [[nodiscard]] vulkan_device final
    {
    public:
        VkPhysicalDevice physical{};
        VkDevice logical{};

        uint32_t graphics_family{};
        uint32_t present_family{};

    public: // Destruction
        ~vulkan_device();
    };

    std::unique_ptr<vulkan_device> create_device(vulkan_context* context);
} // namespace vkpong

#endif // !VKPONG_VULKAN_DEVICE_INCLUDED
