#ifndef VKPONG_VULKAN_DEVICE_INCLUDED
#define VKPONG_VULKAN_DEVICE_INCLUDED

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

namespace vkpong
{
    class vulkan_context;
} // namespace vkpong

namespace vkpong
{
    struct [[nodiscard]] swap_chain_support
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> surface_formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    class [[nodiscard]] vulkan_device final
    {
    public:
        VkPhysicalDevice physical{};
        VkDevice logical{};

        uint32_t graphics_family{};
        uint32_t present_family{};
        swap_chain_support swap_chain_details;

    public: // Destruction
        ~vulkan_device();
    };

    std::unique_ptr<vulkan_device> create_device(vulkan_context* context);
} // namespace vkpong

#endif // !VKPONG_VULKAN_DEVICE_INCLUDED
