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
    public: // Constants
        static constexpr int max_frames_in_flight{2};

    public: // Construction
        vulkan_swap_chain(GLFWwindow* window,
            vulkan_context* context,
            vulkan_device* device);

        vulkan_swap_chain(vulkan_swap_chain const&) = delete;

        vulkan_swap_chain(vulkan_swap_chain&&) noexcept = delete;

    public: // Destruction
        ~vulkan_swap_chain();

    public: // Interface
        [[nodiscard]] constexpr VkExtent2D extent() const;

        [[nodiscard]] constexpr VkFormat image_format() const;

        [[nodiscard]] constexpr VkImage image(uint32_t const image_index) const;

        [[nodiscard]] constexpr VkImageView image_view(
            uint32_t const image_index) const;

        [[nodiscard]] bool is_multisampled() const;

        [[nodiscard]] constexpr VkImageView intermediate_image_view() const;

        [[nodiscard]] bool acquire_next_image(uint32_t current_frame,
            uint32_t& image_index);

        void submit_command_buffer(VkCommandBuffer const* command_buffer,
            uint32_t current_frame,
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
        struct [[nodiscard]] image_sync final
        {
        public: // Data
            VkSemaphore image_available{};
            VkSemaphore render_finished{};
            VkFence in_flight{};

        public: // Construction
            image_sync(vkpong::vulkan_device* const device);

            image_sync(image_sync const&) = delete;
            image_sync(image_sync&&) noexcept;

        public: // Destruction
            ~image_sync();

        public: // Operators
            image_sync& operator=(image_sync const&) = delete;
            image_sync& operator=(image_sync&&) noexcept = delete;

        private: // Data
            vulkan_device* device_{};
        };

        GLFWwindow* window_{};
        vulkan_context* context_{};
        vulkan_device* device_{};
        VkFormat image_format_{};
        VkExtent2D extent_{};
        VkSwapchainKHR chain{};
        std::vector<VkImage> images_;
        std::vector<VkImageView> image_views_;
        std::vector<image_sync> image_syncs_{};
        VkImage color_image_;
        VkDeviceMemory color_image_memory_;
        VkImageView color_image_view_;

        VkQueue graphics_queue_{};
        VkQueue present_queue_{};

        bool framebuffer_resized_{};
    };

} // namespace vkpong

constexpr VkExtent2D vkpong::vulkan_swap_chain::extent() const
{
    return extent_;
}

constexpr VkFormat vkpong::vulkan_swap_chain::image_format() const
{
    return image_format_;
}

constexpr VkImage vkpong::vulkan_swap_chain::image(
    uint32_t const image_index) const
{
    return images_[image_index];
}

constexpr VkImageView vkpong::vulkan_swap_chain::image_view(
    uint32_t const image_index) const
{
    return image_views_[image_index];
}

constexpr VkImageView vkpong::vulkan_swap_chain::intermediate_image_view() const
{
    return color_image_view_;
}

#endif // !VKPONG_VULKAN_SWAP_CHAIN_INCLUDED
