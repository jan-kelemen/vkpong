#ifndef VKPONG_VULKAN_SWAP_CHAIN_INCLUDED
#define VKPONG_VULKAN_SWAP_CHAIN_INCLUDED

#include <vulkan/vulkan_core.h>

#include <memory>
#include <vector>

struct GLFWwindow;

namespace vkpong
{
    class vulkan_context;
    class vulkan_device;
} // namespace vkpong

namespace vkpong
{
    struct [[nodiscard]] swap_chain_support
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> surface_formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    swap_chain_support query_swap_chain_support(VkPhysicalDevice device,
        VkSurfaceKHR surface);

    class [[nodiscard]] vulkan_swap_chain final
    {
    public:
        VkFormat image_format_{};
        VkSwapchainKHR chain{};
        VkExtent2D extent_{};
        std::vector<VkImage> images_;
        std::vector<VkImageView> image_views_;

    public: // Destruction
        ~vulkan_swap_chain();

    private:
        void create_chain_and_images();

    private:
        GLFWwindow* window_{};
        vulkan_context* context_{};
        vulkan_device* device_{};

        friend std::unique_ptr<vulkan_swap_chain> create_swap_chain(GLFWwindow*,
            vulkan_context* context,
            vulkan_device* device);
    };

    std::unique_ptr<vulkan_swap_chain> create_swap_chain(GLFWwindow* window,
        vulkan_context* context,
        vulkan_device* device);
} // namespace vkpong

#endif // !VKPONG_VULKAN_SWAP_CHAIN_INCLUDED
