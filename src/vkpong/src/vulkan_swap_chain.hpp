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

    public: // Construction
        vulkan_swap_chain(GLFWwindow* window,
            vulkan_context* context,
            vulkan_device* device);

        vulkan_swap_chain(vulkan_swap_chain const&) = delete;

        vulkan_swap_chain(vulkan_swap_chain&&) noexcept = delete;

    public: // Destruction
        ~vulkan_swap_chain();

    public: // Interface
        [[nodiscard]] bool acquire_next_image(uint32_t& image_index);

        void submit_command_buffer(VkCommandBuffer const* command_buffer,
            uint32_t image_index);

    public: // Operators
        vulkan_swap_chain& operator=(vulkan_swap_chain const&) = delete;

        vulkan_swap_chain& operator=(vulkan_swap_chain&&) noexcept = delete;

    private: // Helpers
        void create_chain_and_images();
        void cleanup();
        void recreate();

        static void
        framebuffer_resize_callback(GLFWwindow* window, int width, int height);

    private:
        GLFWwindow* window_{};
        vulkan_context* context_{};
        vulkan_device* device_{};
        VkSemaphore image_available_{};
        VkSemaphore render_finished_{};
        VkFence in_flight_{};
        VkQueue graphics_queue_{};
        VkQueue present_queue_{};

        bool framebuffer_resized_{};

        friend std::unique_ptr<vulkan_swap_chain> create_swap_chain(GLFWwindow*,
            vulkan_context* context,
            vulkan_device* device);
    };

    std::unique_ptr<vulkan_swap_chain> create_swap_chain(GLFWwindow* window,
        vulkan_context* context,
        vulkan_device* device);
} // namespace vkpong

#endif // !VKPONG_VULKAN_SWAP_CHAIN_INCLUDED
