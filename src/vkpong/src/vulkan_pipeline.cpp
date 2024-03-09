#include <vulkan_pipeline.hpp>

#include <scope_exit.hpp>
#include <vulkan_device.hpp>
#include <vulkan_swap_chain.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
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

vkpong::vulkan_pipeline::~vulkan_pipeline()
{
    vkDestroyPipeline(device_->logical, pipeline_, nullptr);
    vkDestroyPipelineLayout(device_->logical, layout_, nullptr);
}

std::unique_ptr<vkpong::vulkan_pipeline>
vkpong::create_pipeline(vulkan_device* device, vulkan_swap_chain* swap_chain)
{
    // vert shader
    VkShaderModule const vert_shader_code{
        create_shader_module(device->logical, read_file("vert.spv"))};
    VKPONG_ON_SCOPE_EXIT(
        vkDestroyShaderModule(device->logical, vert_shader_code, nullptr));

    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_code;
    vert_shader_stage_info.pName = "main";

    // frag shader
    VkShaderModule const frag_shader_code{
        create_shader_module(device->logical, read_file("frag.spv"))};
    VKPONG_ON_SCOPE_EXIT(
        vkDestroyShaderModule(device->logical, frag_shader_code, nullptr));

    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_code;
    frag_shader_stage_info.pName = "main";

    std::array const shader_stages{vert_shader_stage_info,
        frag_shader_stage_info};

    // vertex input
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // viewport state
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    // rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    // multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    // TODO-JK: enable anti-aliasing

    // color blend state
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

    // TODO-JK: depth stencil

    // dynamic states
    std::array<VkDynamicState, 2> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    dynamic_state.dynamicStateCount =
        static_cast<uint32_t>(dynamic_states.size()),
    dynamic_state.pDynamicStates = dynamic_states.data();

    auto rv{std::make_unique<vulkan_pipeline>()};
    rv->device_ = device;

    // pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(device->logical,
            &pipeline_layout_info,
            nullptr,
            &rv->layout_) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create pipeline layout!"};
    }

    VkPipelineRenderingCreateInfoKHR rendering_create_info{};
    rendering_create_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    rendering_create_info.colorAttachmentCount = 1;
    rendering_create_info.pColorAttachmentFormats = &swap_chain->image_format_;

    VkGraphicsPipelineCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.pVertexInputState = &vertex_input_info;
    create_info.pInputAssemblyState = &input_assembly;
    create_info.pRasterizationState = &rasterizer;
    create_info.pColorBlendState = &color_blending;
    create_info.pMultisampleState = &multisampling;
    create_info.pViewportState = &viewport_state;
    create_info.pDynamicState = &dynamic_state;
    create_info.stageCount = static_cast<uint32_t>(shader_stages.size());
    create_info.pStages = shader_stages.data();
    create_info.layout = rv->layout_;

    create_info.pNext = &rendering_create_info;
    if (vkCreateGraphicsPipelines(device->logical,
            VK_NULL_HANDLE,
            1,
            &create_info,
            nullptr,
            &rv->pipeline_) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create pipeline!"};
    }

    return rv;
}
