#ifndef VKPONG_VULKAN_PIPELINE_INCLUDED
#define VKPONG_VULKAN_PIPELINE_INCLUDED

#include <glm/glm.hpp>

#include <vulkan/vulkan_core.h>

#include <memory>

namespace
{
    struct [[nodiscard]] push_consts
    {
        glm::fvec4 color[3];
    };
} // namespace

namespace vkpong
{
    class vulkan_device;
    class vulkan_swap_chain;
} // namespace vkpong

namespace vkpong
{
    class [[nodiscard]] vulkan_pipeline final
    {
    public: // Destruction
        ~vulkan_pipeline();

    public: // Interface
        [[nodiscard]] constexpr VkPipeline pipeline() const;

        [[nodiscard]] constexpr VkPipelineLayout layout() const;

    private: // Data
        vulkan_device* device_{};
        VkPipelineLayout layout_{};
        VkPipeline pipeline_{};

        friend std::unique_ptr<vulkan_pipeline>
        create_pipeline(vulkan_device* device, vulkan_swap_chain* swap_chain);
    };

    std::unique_ptr<vulkan_pipeline> create_pipeline(vulkan_device* device,
        vulkan_swap_chain* swap_chain);
} // namespace vkpong

constexpr VkPipeline vkpong::vulkan_pipeline::pipeline() const
{
    return pipeline_;
}

constexpr VkPipelineLayout vkpong::vulkan_pipeline::layout() const
{
    return layout_;
}

#endif // !VKPONG_VULKAN_PIPELINE_INCLUDED
