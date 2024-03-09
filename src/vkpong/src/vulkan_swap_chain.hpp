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
    class [[nodiscard]] swap_chain final
    {
    public:

    public: // Destruction
        ~swap_chain();

    private:
        vulkan_device* device_{};
        VkSwapchainKHR chain{};
        VkFormat image_format_{};
        VkExtent2D extent_{};
        std::vector<VkImage> images_;
        std::vector<VkImageView> image_views_;

        friend std::unique_ptr<swap_chain> create_swap_chain(GLFWwindow*,
            vulkan_context* context,
            vulkan_device* device);
    };

    std::unique_ptr<swap_chain> create_swap_chain(GLFWwindow* window,
        vulkan_context* context,
        vulkan_device* device);
} // namespace vkpong

#endif // !VKPONG_VULKAN_SWAP_CHAIN_INCLUDED
