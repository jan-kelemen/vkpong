#ifndef VKPONG_VULKAN_PIPELINE_INCLUDED
#define VKPONG_VULKAN_PIPELINE_INCLUDED

#include <vulkan/vulkan_core.h>

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace vkpong
{
    class vulkan_device;
    class vulkan_swap_chain;
} // namespace vkpong

namespace vkpong
{
    class [[nodiscard]] vulkan_pipeline final
    {
    public: // Construction
        vulkan_pipeline(vulkan_device* device,
            VkPipelineLayout pipeline_layout,
            VkPipeline pipeline);

        vulkan_pipeline(vulkan_pipeline const&) = delete;

        vulkan_pipeline(vulkan_pipeline&& other) noexcept;

    public: // Destruction
        ~vulkan_pipeline();

    public: // Interface
        [[nodiscard]] constexpr VkPipeline pipeline() const noexcept;

        [[nodiscard]] constexpr VkPipelineLayout
        pipeline_layout() const noexcept;

    public: // Operators
        vulkan_pipeline& operator=(vulkan_pipeline const&) = delete;

        vulkan_pipeline& operator=(vulkan_pipeline&& other) noexcept;

    private: // Data
        vulkan_device* device_{};
        VkPipelineLayout pipeline_layout_{};
        VkPipeline pipeline_{};
    };

    class [[nodiscard]] vulkan_pipeline_builder final
    {
    public: // Construction
        vulkan_pipeline_builder(vulkan_device* device, VkFormat image_format);

    public: // Destruction
        ~vulkan_pipeline_builder();

    public: // Interface
        [[nodiscard]] vulkan_pipeline build();

        vulkan_pipeline_builder& add_shader(VkShaderStageFlagBits stage,
            std::filesystem::path const& path,
            std::string_view entry_point);

        vulkan_pipeline_builder& add_vertex_input(
            std::span<VkVertexInputBindingDescription const>
                binding_description,
            std::span<VkVertexInputAttributeDescription const>
                attribute_descriptions);

        vulkan_pipeline_builder& add_descriptor_set_layout(
            VkDescriptorSetLayout descriptor_set_layout);

        vulkan_pipeline_builder& with_rasterization_samples(
            VkSampleCountFlagBits samples);

        vulkan_pipeline_builder& with_push_constants(
            VkPushConstantRange push_constants);

    private: // Helpers
        void cleanup();

    private: // Data
        vulkan_device* device_{};
        VkFormat image_format_{};
        std::vector<
            std::tuple<VkShaderStageFlagBits, VkShaderModule, std::string>>
            shaders_;
        std::vector<VkVertexInputBindingDescription> vertex_input_binding_;
        std::vector<VkVertexInputAttributeDescription> vertex_input_attributes_;
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts_;
        VkSampleCountFlagBits rasterization_samples_{VK_SAMPLE_COUNT_1_BIT};
        std::optional<VkPushConstantRange> push_constants_;
    };
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

#endif // !VKPONG_VULKAN_PIPELINE_INCLUDED
