#ifndef VKPONG_VULKAN_PIPELINE_INCLUDED
#define VKPONG_VULKAN_PIPELINE_INCLUDED

#include <vulkan/vulkan_core.h>

#include <memory>

namespace vkpong
{
    class vulkan_device;
    class vulkan_swap_chain;
} // namespace vkpong

namespace vkpong
{
    class [[nodiscard]] vulkan_pipeline
    {
    public:
        ~vulkan_pipeline();

    private:
        vulkan_device* device_{};
        VkPipelineLayout layout_{};
        VkPipeline pipeline_{};

        friend std::unique_ptr<vulkan_pipeline>
        create_pipeline(vulkan_device* device, vulkan_swap_chain* swap_chain);
    };

    std::unique_ptr<vulkan_pipeline> create_pipeline(vulkan_device* device,
        vulkan_swap_chain* swap_chain);
} // namespace vkpong

#endif // !VKPONG_VULKAN_PIPELINE_INCLUDED
