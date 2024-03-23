#include <vulkan_pipeline.hpp>

#include <vulkan_device.hpp>
#include <vulkan_utility.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    [[nodiscard]] std::vector<char> read_file(std::filesystem::path const& file)
    {
        std::ifstream stream{file, std::ios::ate | std::ios::binary};

        if (!stream.is_open())
        {
            throw std::runtime_error{"failed to open file!"};
        }

        auto const eof{stream.tellg()};

        std::vector<char> buffer(static_cast<size_t>(eof));
        stream.seekg(0);

        stream.read(buffer.data(), eof);

        return buffer;
    }
} // namespace

namespace
{
    [[nodiscard]] VkShaderModule create_shader_module(VkDevice device,
        std::span<char const> code)
    {
        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<uint32_t const*>(code.data());

        // TODO-JK: maintenance5 feature
        VkShaderModule module{};
        if (vkCreateShaderModule(device, &create_info, nullptr, &module) !=
            VK_SUCCESS)
        {
            throw std::runtime_error{"failed to create shader module"};
        }

        return module;
    }
} // namespace

vkpong::vulkan_pipeline::vulkan_pipeline(vulkan_device* device,
    VkPipelineLayout pipeline_layout,
    VkPipeline pipeline)
    : device_{device}
    , pipeline_layout_{pipeline_layout}
    , pipeline_{pipeline}
{
}

vkpong::vulkan_pipeline::vulkan_pipeline(vulkan_pipeline&& other) noexcept
    : device_{std::exchange(other.device_, nullptr)}
    , pipeline_layout_{std::exchange(other.pipeline_layout_, nullptr)}
    , pipeline_{std::exchange(other.pipeline_, nullptr)}
{
}

vkpong::vulkan_pipeline::~vulkan_pipeline()
{
    if (device_)
    {
        vkDestroyPipeline(device_->logical(), pipeline_, nullptr);
        vkDestroyPipelineLayout(device_->logical(), pipeline_layout_, nullptr);
    }
}

vkpong::vulkan_pipeline& vkpong::vulkan_pipeline::operator=(
    vulkan_pipeline&& other) noexcept
{
    using std::swap;

    if (this != &other)
    {
        swap(device_, other.device_);
        swap(pipeline_layout_, other.pipeline_layout_);
        swap(pipeline_, other.pipeline_);
    }

    return *this;
}

vkpong::vulkan_pipeline_builder::vulkan_pipeline_builder(
    vulkan_device* const device,
    VkFormat const image_format)
    : device_{device}
    , image_format_{image_format}
{
}

vkpong::vulkan_pipeline_builder::~vulkan_pipeline_builder() { cleanup(); }

vkpong::vulkan_pipeline vkpong::vulkan_pipeline_builder::build()
{
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    shader_stages.reserve(shaders_.size());
    for (auto const& shader : shaders_)
    {
        VkPipelineShaderStageCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        create_info.stage = std::get<0>(shader);
        create_info.module = std::get<1>(shader);
        create_info.pName = std::get<2>(shader).c_str();

        shader_stages.push_back(create_info);
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount =
        count_cast(vertex_input_binding_.size());
    vertex_input_info.pVertexBindingDescriptions = vertex_input_binding_.data();
    vertex_input_info.vertexAttributeDescriptionCount =
        count_cast(vertex_input_attributes_.size());
    vertex_input_info.pVertexAttributeDescriptions =
        vertex_input_attributes_.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.minSampleShading = .2f;
    multisampling.rasterizationSamples = rasterization_samples_;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.blendEnable = VK_FALSE,
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    std::ranges::fill(color_blending.blendConstants, 0.0f);

    constexpr std::array dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    dynamic_state.dynamicStateCount = count_cast(dynamic_states.size()),
    dynamic_state.pDynamicStates = dynamic_states.data();

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (push_constants_)
    {
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &(*push_constants_);
    }

    pipeline_layout_info.setLayoutCount =
        count_cast(descriptor_set_layouts_.size());
    pipeline_layout_info.pSetLayouts = descriptor_set_layouts_.data();

    VkPipelineLayout pipeline_layout{};
    if (vkCreatePipelineLayout(device_->logical(),
            &pipeline_layout_info,
            nullptr,
            &pipeline_layout) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create pipeline layout!"};
    }

    VkPipelineRenderingCreateInfoKHR rendering_create_info{};
    rendering_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    rendering_create_info.colorAttachmentCount = 1;
    rendering_create_info.pColorAttachmentFormats = &image_format_;

    VkGraphicsPipelineCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.pVertexInputState = &vertex_input_info;
    create_info.pInputAssemblyState = &input_assembly;
    create_info.pRasterizationState = &rasterizer;
    create_info.pColorBlendState = &color_blending;
    create_info.pMultisampleState = &multisampling;
    create_info.pViewportState = &viewport_state;
    create_info.pDynamicState = &dynamic_state;
    create_info.stageCount = count_cast(shader_stages.size());
    create_info.pStages = shader_stages.data();
    create_info.layout = pipeline_layout;
    create_info.pNext = &rendering_create_info;

    VkPipeline pipeline{};
    if (vkCreateGraphicsPipelines(device_->logical(),
            VK_NULL_HANDLE,
            1,
            &create_info,
            nullptr,
            &pipeline) != VK_SUCCESS)
    {
        vkDestroyPipelineLayout(device_->logical(), pipeline_layout, nullptr);
        throw std::runtime_error{"failed to create pipeline!"};
    }

    cleanup();

    return {device_, pipeline_layout, pipeline};
}

vkpong::vulkan_pipeline_builder& vkpong::vulkan_pipeline_builder::add_shader(
    VkShaderStageFlagBits const stage,
    std::filesystem::path const& path,
    std::string_view entry_point)
{
    std::string name{entry_point};
    shaders_.reserve(shaders_.size() + 1);

    shaders_.emplace_back(stage,
        create_shader_module(device_->logical(), read_file(path)),
        std::move(name));
    return *this;
}

vkpong::vulkan_pipeline_builder&
vkpong::vulkan_pipeline_builder::add_vertex_input(
    std::span<VkVertexInputBindingDescription const> binding_descriptions,
    std::span<VkVertexInputAttributeDescription const> attribute_descriptions)
{
    vertex_input_binding_.reserve(
        vertex_input_binding_.size() + binding_descriptions.size());
    vertex_input_attributes_.reserve(
        vertex_input_attributes_.size() + attribute_descriptions.size());

    vertex_input_binding_.insert(vertex_input_binding_.cend(),
        binding_descriptions.cbegin(),
        binding_descriptions.cend());
    vertex_input_attributes_.insert(vertex_input_attributes_.cend(),
        attribute_descriptions.cbegin(),
        attribute_descriptions.cend());
    return *this;
}

vkpong::vulkan_pipeline_builder&
vkpong::vulkan_pipeline_builder::add_descriptor_set_layout(
    VkDescriptorSetLayout const descriptor_set_layout)
{
    descriptor_set_layouts_.emplace_back(descriptor_set_layout);
    return *this;
}

vkpong::vulkan_pipeline_builder&
vkpong::vulkan_pipeline_builder::with_rasterization_samples(
    VkSampleCountFlagBits const samples)
{
    rasterization_samples_ = samples;
    return *this;
}

vkpong::vulkan_pipeline_builder&
vkpong::vulkan_pipeline_builder::with_push_constants(
    VkPushConstantRange const push_constants)
{
    push_constants_ = push_constants;

    return *this;
}

void vkpong::vulkan_pipeline_builder::cleanup()
{
    descriptor_set_layouts_.clear();
    vertex_input_attributes_.clear();
    vertex_input_binding_.clear();

    for (auto const& shader : shaders_)
    {
        vkDestroyShaderModule(device_->logical(), std::get<1>(shader), nullptr);
    }

    shaders_.clear();
}
