#ifndef VKPONG_VULKAN_RENDERER_INCLUDED
#define VKPONG_VULKAN_RENDERER_INLCUDED

#include <vulkan/vulkan_core.h>

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
    class [[nodiscard]] vulkan_renderer
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

    private: // Types
        struct [[nodiscard]] mapped_buffer final
        {
        public: // Data
            VkBuffer buffer{};
            VkDeviceMemory device_memory{};
            void* mapped_memory{};

        public: // Construction
            mapped_buffer(vulkan_device* device, size_t size);

            mapped_buffer(mapped_buffer const&) = delete;
            mapped_buffer(mapped_buffer&& other) noexcept;

        public: // Destruction
            ~mapped_buffer();

        public: // Operators
            mapped_buffer& operator=(mapped_buffer const&) = delete;
            mapped_buffer& operator=(mapped_buffer&& other) noexcept = delete;

        private:
            vulkan_device* device_;
        };

    private: // Helpers
        void record_command_buffer(VkCommandBuffer& command_buffer,
            VkDescriptorSet const& descriptor_set,
            uint32_t image_index);

        void update_uniform_buffer(mapped_buffer& buffer);

    private: // Data
        std::unique_ptr<vulkan_context> context_;
        std::unique_ptr<vulkan_device> device_;
        std::unique_ptr<vulkan_swap_chain> swap_chain_;
        std::unique_ptr<vulkan_pipeline> pipeline_;

        VkCommandPool command_pool_{};
        std::vector<VkCommandBuffer> command_buffers_{};

        VkBuffer buffer_{};
        VkDeviceMemory buffer_memory_{};

        std::vector<mapped_buffer> uniform_buffers_;

        VkDescriptorPool descriptor_pool_{};
        std::vector<VkDescriptorSet> descriptor_sets_;

        uint32_t current_frame_{};
    };
} // namespace vkpong

#endif // !VKPONG_VULKAN_RENDERER_INCLUDED
