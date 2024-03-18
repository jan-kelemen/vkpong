#ifndef VKPONG_VULKAN_SWAP_CHAIN_INCLUDED
#define VKPONG_VULKAN_SWAP_CHAIN_INCLUDED

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

struct GLFWwindow;

namespace vkpong
{
    class vulkan_context;
    class vulkan_device;
} // namespace vkpong

namespace vkpong
{
    struct [[nodiscard]] swap_chain_support final
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

        vulkan_swap_chain(vulkan_swap_chain&& other) noexcept;

    public: // Destruction
        ~vulkan_swap_chain();

    public: // Interface
        [[nodiscard]] constexpr VkExtent2D extent() const noexcept;

        [[nodiscard]] constexpr VkFormat image_format() const noexcept;

        [[nodiscard]] constexpr VkImage image(
            uint32_t image_index) const noexcept;

        [[nodiscard]] constexpr VkImageView image_view(
            uint32_t image_index) const noexcept;

        [[nodiscard]] bool acquire_next_image(uint32_t current_frame,
            uint32_t& image_index);

        [[nodiscard]] bool submit_command_buffer(
            VkCommandBuffer const* command_buffer,
            uint32_t current_frame,
            uint32_t image_index);

        void recreate();

    public: // Operators
        vulkan_swap_chain& operator=(vulkan_swap_chain const&) = delete;

        vulkan_swap_chain& operator=(vulkan_swap_chain&& other) noexcept;

    private: // Helpers
        void create_chain_and_images();

        void cleanup();

    private:
        struct [[nodiscard]] image_sync final
        {
        public: // Data
            vulkan_device* device_{};
            VkSemaphore image_available{};
            VkSemaphore render_finished{};
            VkFence in_flight{};

        public: // Construction
            explicit image_sync(vkpong::vulkan_device* device);

            image_sync(image_sync const&) = delete;
            image_sync(image_sync&& other) noexcept;

        public: // Destruction
            ~image_sync();

        public: // Operators
            image_sync& operator=(image_sync const&) = delete;
            image_sync& operator=(image_sync&&) noexcept = delete;
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

        VkQueue graphics_queue_{};
        VkQueue present_queue_{};
    };

} // namespace vkpong

inline constexpr VkExtent2D vkpong::vulkan_swap_chain::extent() const noexcept
{
    return extent_;
}

inline constexpr VkFormat
vkpong::vulkan_swap_chain::image_format() const noexcept
{
    return image_format_;
}

inline constexpr VkImage vkpong::vulkan_swap_chain::image(
    uint32_t const image_index) const noexcept
{
    return images_[image_index];
}

inline constexpr VkImageView vkpong::vulkan_swap_chain::image_view(
    uint32_t const image_index) const noexcept
{
    return image_views_[image_index];
}

#endif // !VKPONG_VULKAN_SWAP_CHAIN_INCLUDED
