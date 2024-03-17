#ifndef VKPONG_VULKAN_RENDERER_INCLUDED
#define VKPONG_VULKAN_RENDERER_INCLUDED

#include <vulkan_buffer.hpp>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace vkpong
{
    class vulkan_context;
    class vulkan_device;
    class vulkan_swap_chain;
    class vulkan_pipeline;
} // namespace vkpong

namespace vkpong
{
    class [[nodiscard]] vulkan_renderer final
    {
    public: // Construction
        vulkan_renderer(std::unique_ptr<vulkan_context> context,
            std::unique_ptr<vulkan_device> device,
            std::unique_ptr<vulkan_swap_chain> swap_chain);

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
        void record_command_buffer(VkCommandBuffer& command_buffer,
            VkDescriptorSet const& descriptor_set,
            uint32_t image_index);

        void update_uniform_buffer(vulkan_buffer& buffer);
        void update_instance_buffer(vulkan_buffer& buffer);

        [[nodiscard]] bool is_multisampled() const;

    private: // Data
        std::unique_ptr<vulkan_context> context_;
        std::unique_ptr<vulkan_device> device_;
        std::unique_ptr<vulkan_swap_chain> swap_chain_;
        std::unique_ptr<vulkan_pipeline> pipeline_;

        VkImage color_image_{};
        VkDeviceMemory color_image_memory_{};
        VkImageView color_image_view_{};

        VkCommandPool command_pool_{};
        std::vector<VkCommandBuffer> command_buffers_{};

        vulkan_buffer vertex_and_index_buffer_;
        std::vector<vulkan_buffer> instance_buffers_;
        std::vector<vulkan_buffer> uniform_buffers_;

        VkDescriptorSetLayout descriptor_set_layout_{};
        VkDescriptorPool descriptor_pool_{};
        std::vector<VkDescriptorSet> descriptor_sets_;

        uint32_t current_frame_{};
    };
} // namespace vkpong

#endif // !VKPONG_VULKAN_RENDERER_INCLUDED
