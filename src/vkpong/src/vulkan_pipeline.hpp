#ifndef VKPONG_VULKAN_PIPELINE_INCLUDED
#define VKPONG_VULKAN_PIPELINE_INCLUDED

#include <glm/glm.hpp>

#include <vulkan/vulkan_core.h>

#include <array>
#include <memory>
#include <span>
#include <vector>

namespace
{
    struct [[nodiscard]] push_consts final
    {
        glm::fvec4 color[6];
    };

    struct [[nodiscard]] vertex final
    {
        glm::fvec2 position;

        [[nodiscard]] static constexpr auto binding_description()
        {
            return VkVertexInputBindingDescription{.binding = 0,
                .stride = sizeof(vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
        }

        [[nodiscard]] static constexpr auto attribute_descriptions()
        {
            constexpr std::array descriptions{
                VkVertexInputAttributeDescription{.location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(vertex, position)}};

            return descriptions;
        }
    };

    std::vector<vertex> const vertices{{{-.5f, -.5f}},
        {{.5f, -.5f}},
        {{.5f, .5f}},
        {{-.5f, .5f}}};

    std::vector<uint16_t> const indices{0, 1, 2, 2, 3, 0};

    struct [[nodiscard]] uniform_buffer_object final
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
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
        [[nodiscard]] constexpr VkPipeline pipeline() const noexcept;

        [[nodiscard]] constexpr VkPipelineLayout
        pipeline_layout() const noexcept;

        [[nodiscard]] constexpr VkDescriptorSetLayout
        descriptor_set_layout() const noexcept;

    private: // Data
        vulkan_device* device_{};
        VkDescriptorSetLayout descriptor_set_layout_{};
        VkPipelineLayout pipeline_layout_{};
        VkPipeline pipeline_{};

        friend std::unique_ptr<vulkan_pipeline>
        create_pipeline(vulkan_device* device, vulkan_swap_chain* swap_chain);
    };

    std::unique_ptr<vulkan_pipeline> create_pipeline(vulkan_device* device,
        vulkan_swap_chain* swap_chain);
} // namespace vkpong

inline constexpr VkPipeline vkpong::vulkan_pipeline::pipeline() const noexcept
{
    return pipeline_;
}

inline constexpr VkPipelineLayout
vkpong::vulkan_pipeline::pipeline_layout() const noexcept
{
    return pipeline_layout_;
}

inline constexpr VkDescriptorSetLayout
vkpong::vulkan_pipeline::descriptor_set_layout() const noexcept
{
    return descriptor_set_layout_;
}

#endif // !VKPONG_VULKAN_PIPELINE_INCLUDED
