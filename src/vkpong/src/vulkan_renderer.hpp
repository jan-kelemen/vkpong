#ifndef VKPONG_VULKAN_RENDERER_INCLUDED
#define VKPONG_VULKAN_RENDERER_INLCUDED

#include <vulkan/vulkan_core.h>

#include <memory>

namespace vkpong
{
    class vulkan_context;
    class vulkan_device;
    class vulkan_swap_chain;
    class vulkan_pipeline;
} // namespace vkpong

namespace vkpong
{
    class [[nodiscard]] vulkan_renderer
    {
    public: // Construction
        vulkan_renderer(std::unique_ptr<vulkan_context> context,
            std::unique_ptr<vulkan_device> device,
            std::unique_ptr<vulkan_swap_chain> swap_chain,
            std::unique_ptr<vulkan_pipeline> pipeline);

        vulkan_renderer(vulkan_renderer const&) = delete;

        vulkan_renderer(vulkan_renderer&&) noexcept = delete;

    public: // Destruction
        ~vulkan_renderer();

    public: // Interface
        void draw();

    public: // Operators
        vulkan_renderer& operator=(vulkan_renderer const&) = delete;

        vulkan_renderer& operator=(vulkan_renderer&&) noexcept = delete;

    private: // Helpers
        void record_command_buffer(uint32_t image_index);

    private: // Data
        std::unique_ptr<vulkan_context> context_;
        std::unique_ptr<vulkan_device> device_;
        std::unique_ptr<vulkan_swap_chain> swap_chain_;
        std::unique_ptr<vulkan_pipeline> pipeline_;
        VkCommandPool command_pool_{};
        VkCommandBuffer command_buffer_{};
    };
} // namespace vkpong

#endif // !VKPONG_VULKAN_RENDERER_INCLUDED
