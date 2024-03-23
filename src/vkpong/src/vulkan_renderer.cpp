#include <vulkan_renderer.hpp>

#include <game.hpp>
#include <vulkan_context.hpp>
#include <vulkan_device.hpp>
#include <vulkan_pipeline.hpp>
#include <vulkan_swap_chain.hpp>
#include <vulkan_utility.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cassert>
#include <span>
#include <stdexcept>

namespace
{
    struct [[nodiscard]] push_consts final
    {
        glm::fvec4 color[6];
    };

    struct [[nodiscard]] ball_push_consts final
    {
        glm::fvec4 color[6];
        glm::fvec2 resolution;
    };

    struct [[nodiscard]] instance_data final
    {
        glm::fvec2 offset;
        glm::fvec2 dimension;
        glm::fvec3 color;
    };

    struct [[nodiscard]] vertex final
    {
        glm::fvec2 position;

        [[nodiscard]] static constexpr auto binding_description()
        {
            constexpr std::array descriptions{
                VkVertexInputBindingDescription{.binding = 0,
                    .stride = sizeof(vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX},
                VkVertexInputBindingDescription{.binding = 1,
                    .stride = sizeof(instance_data),
                    .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE},
            };

            return descriptions;
        }

        [[nodiscard]] static constexpr auto attribute_descriptions()
        {
            constexpr std::array descriptions{
                VkVertexInputAttributeDescription{.location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(vertex, position)},
                VkVertexInputAttributeDescription{.location = 1,
                    .binding = 1,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(instance_data, offset)},
                VkVertexInputAttributeDescription{.location = 2,
                    .binding = 1,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(instance_data, dimension)},
                VkVertexInputAttributeDescription{.location = 3,
                    .binding = 1,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(instance_data, color)},
            };

            return descriptions;
        }
    };

    std::vector<vertex> const vertices{{{-1, -1}},
        {{1, -1}},
        {{1, 1}},
        {{-1, 1}}};

    std::vector<uint16_t> const indices{0, 1, 2, 2, 3, 0};

    struct [[nodiscard]] uniform_buffer_object final
    {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };
} // namespace

namespace
{
    [[nodiscard]] VkCommandPool create_command_pool(
        vkpong::vulkan_device* const device)
    {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        pool_info.queueFamilyIndex = device->graphics_family();

        VkCommandPool rv{};
        if (vkCreateCommandPool(device->logical(), &pool_info, nullptr, &rv) !=
            VK_SUCCESS)
        {
            throw std::runtime_error{"failed to create command pool"};
        }

        return rv;
    }

    void create_command_buffers(vkpong::vulkan_device* const device,
        VkCommandPool const command_pool,
        uint32_t const count,
        std::span<VkCommandBuffer> data_buffer)
    {
        assert(data_buffer.size() >= count);

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = count;

        if (vkAllocateCommandBuffers(device->logical(),
                &alloc_info,
                data_buffer.data()) != VK_SUCCESS)
        {
            throw std::runtime_error{"failed to allocate command buffers!"};
        }
    }

    VkDescriptorPool create_descriptor_pool(vkpong::vulkan_device* const device)
    {
        constexpr auto count{vkpong::count_cast(
            vkpong::vulkan_swap_chain::max_frames_in_flight)};

        VkDescriptorPoolSize uniform_buffer_pool_size{};
        uniform_buffer_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_buffer_pool_size.descriptorCount = count;

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = &uniform_buffer_pool_size;
        pool_info.maxSets = count;

        VkDescriptorPool rv{};
        if (vkCreateDescriptorPool(device->logical(),
                &pool_info,
                nullptr,
                &rv) != VK_SUCCESS)
        {
            throw std::runtime_error{"failed to create descriptor pool!"};
        }

        return rv;
    }

    [[nodiscard]] VkDescriptorSetLayout create_descriptor_set_layout(
        vkpong::vulkan_device* const device)
    {
        VkDescriptorSetLayoutBinding ubo_layout_binding{};
        ubo_layout_binding.binding = 0;
        ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_layout_binding.descriptorCount = 1;
        ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = 1;
        layout_info.pBindings = &ubo_layout_binding;

        VkDescriptorSetLayout rv{};
        if (vkCreateDescriptorSetLayout(device->logical(),
                &layout_info,
                nullptr,
                &rv) != VK_SUCCESS)
        {
            throw std::runtime_error{"failed to create descriptor set layout"};
        }

        return rv;
    }

    void create_descriptor_sets(vkpong::vulkan_device* const device,
        VkDescriptorSetLayout const& layout,
        VkDescriptorPool const& descriptor_pool,
        std::span<VkDescriptorSet> descriptor_sets)
    {
        constexpr auto count{vkpong::count_cast(
            vkpong::vulkan_swap_chain::max_frames_in_flight)};

        assert(descriptor_sets.size() >= count);

        std::vector<VkDescriptorSetLayout> layouts(count, layout);

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = descriptor_pool;
        alloc_info.descriptorSetCount = count;
        alloc_info.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device->logical(),
                &alloc_info,
                descriptor_sets.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }

    void bind_descriptor_set(vkpong::vulkan_device* const device,
        VkDescriptorSet const& descriptor_set,
        VkBuffer const& buffer)
    {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = buffer;
        buffer_info.offset = 0;
        buffer_info.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_set;
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(device->logical(),
            1,
            &descriptor_write,
            0,
            nullptr);
    }

    void transition_image(VkImage const image,
        VkCommandBuffer const command_buffer,
        VkImageLayout const old_layout,
        VkImageLayout const new_layout)
    {
        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE,
        barrier.srcAccessMask = VK_ACCESS_2_NONE,
        barrier.oldLayout = old_layout, barrier.newLayout = new_layout,
        barrier.image = image,
        barrier.subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.imageMemoryBarrierCount = 1;
        dependency.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(command_buffer, &dependency);
    }
} // namespace

vkpong::vulkan_renderer::vulkan_renderer(vulkan_context* context,
    vulkan_device* device,
    vulkan_swap_chain* swap_chain)
    : context_{context}
    , device_{device}
    , swap_chain_{swap_chain}
    , command_pool_{create_command_pool(device)}
    , command_buffers_{vulkan_swap_chain::max_frames_in_flight}
    , vertex_and_index_buffer_{device,
          sizeof(vertices[0]) * vertices.size() +
              sizeof(indices[0]) * indices.size(),
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT}
    , descriptor_set_layout_{create_descriptor_set_layout(device)}
    , descriptor_pool_{create_descriptor_pool(device)}
{
    pipeline_ = std::make_unique<vulkan_pipeline>(
        vulkan_pipeline_builder{device_, swap_chain_->image_format()}
            .add_shader(VK_SHADER_STAGE_VERTEX_BIT, "vert.spv", "main")
            .add_shader(VK_SHADER_STAGE_FRAGMENT_BIT, "frag.spv", "main")
            .with_rasterization_samples(device_->max_msaa_samples())
            .add_vertex_input(vertex::binding_description(),
                vertex::attribute_descriptions())
            .add_descriptor_set_layout(descriptor_set_layout_)
            .build());

    ball_pipeline_ = std::make_unique<vulkan_pipeline>(
        vulkan_pipeline_builder{device_, swap_chain_->image_format()}
            .add_shader(VK_SHADER_STAGE_VERTEX_BIT, "vert.spv", "main")
            .add_shader(VK_SHADER_STAGE_FRAGMENT_BIT, "ball.spv", "main")
            .with_rasterization_samples(device_->max_msaa_samples())
            .add_vertex_input(vertex::binding_description(),
                vertex::attribute_descriptions())
            .with_push_constants(
                VkPushConstantRange{.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(ball_push_consts)})
            .add_descriptor_set_layout(descriptor_set_layout_)
            .build());

    recreate_images();

    create_command_buffers(device_,
        command_pool_,
        vulkan_swap_chain::max_frames_in_flight,
        command_buffers_);

    descriptor_sets_.resize(vulkan_swap_chain::max_frames_in_flight);
    create_descriptor_sets(device_,
        descriptor_set_layout_,
        descriptor_pool_,
        descriptor_sets_);

    for (size_t i{}; i != size_t{vulkan_swap_chain::max_frames_in_flight}; ++i)
    {
        instance_buffers_.emplace_back(device_,
            sizeof(instance_data) * 3,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        auto const& buffer{uniform_buffers_.emplace_back(device_,
            sizeof(uniform_buffer_object),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            true)};

        bind_descriptor_set(device_, descriptor_sets_[i], buffer.buffer());
    }
}

vkpong::vulkan_renderer::~vulkan_renderer()
{
    vkDeviceWaitIdle(device_->logical());

    vkDestroyDescriptorPool(device_->logical(), descriptor_pool_, nullptr);
    vkDestroyDescriptorSetLayout(device_->logical(),
        descriptor_set_layout_,
        nullptr);

    uniform_buffers_.clear();

    instance_buffers_.clear();

    vkDestroyCommandPool(device_->logical(), command_pool_, nullptr);

    cleanup_images();
}

void vkpong::vulkan_renderer::draw(vkpong::game const& state)
{
    uint32_t image_index{};
    if (!swap_chain_->acquire_next_image(current_frame_, image_index))
    {
        recreate_images();
        return;
    }

    auto& command_buffer{command_buffers_[current_frame_]};
    auto const& descriptor_set{descriptor_sets_[current_frame_]};

    vkResetCommandBuffer(command_buffer, 0);

    record_command_buffer(command_buffer, descriptor_set, image_index);

    update_uniform_buffer(uniform_buffers_[current_frame_]);
    update_instance_buffer(state, instance_buffers_[current_frame_]);

    if (!swap_chain_->submit_command_buffer(&command_buffer,
            current_frame_,
            image_index))
    {
        recreate_images();
    }

    current_frame_ =
        (current_frame_ + 1) % vulkan_swap_chain::max_frames_in_flight;
}

void vkpong::vulkan_renderer::record_command_buffer(
    VkCommandBuffer& command_buffer,
    VkDescriptorSet const& descriptor_set,
    uint32_t const image_index)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
    {
        throw std::runtime_error{"unable to begin command buffer recording!"};
    }

    transition_image(swap_chain_->image(image_index),
        command_buffer,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    constexpr VkClearValue clear_value{{{0.0f, 4.0f, 0.0f, 1.0f}}};
    VkRenderingAttachmentInfoKHR color_attachment_info{};
    color_attachment_info.sType =
        VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color_attachment_info.imageLayout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_info.clearValue = clear_value;
    if (is_multisampled())
    {
        color_attachment_info.imageView = color_image_view_;
        color_attachment_info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        color_attachment_info.resolveImageView =
            swap_chain_->image_view(image_index);
        color_attachment_info.resolveImageLayout =
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    else
    {
        color_attachment_info.imageView = swap_chain_->image_view(image_index);
    }

    VkRenderingInfoKHR render_info{};
    render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    render_info.renderArea = {{0, 0}, swap_chain_->extent()};
    render_info.layerCount = 1;
    render_info.colorAttachmentCount = 1;
    render_info.pColorAttachments = &color_attachment_info;

    vkCmdBeginRendering(command_buffer, &render_info);

    vkCmdBindPipeline(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_->pipeline());

    size_t const vertices_size{sizeof(vertices[0]) * vertices.size()};
    vertex_and_index_buffer_.fill(0, as_bytes(vertices));
    vertex_and_index_buffer_.fill(vertices_size, as_bytes(indices));

    std::array vertex_buffer{vertex_and_index_buffer_.buffer()};
    std::array instance_buffer{instance_buffers_[current_frame_].buffer()};
    std::array const offsets{VkDeviceSize{0}};
    vkCmdBindVertexBuffers(command_buffer,
        0,
        1,
        vertex_buffer.data(),
        offsets.data());
    vkCmdBindVertexBuffers(command_buffer,
        1,
        1,
        instance_buffer.data(),
        offsets.data());

    vkCmdBindIndexBuffer(command_buffer,
        vertex_and_index_buffer_.buffer(),
        vertices_size,
        VK_INDEX_TYPE_UINT16);

    VkExtent2D const extent{swap_chain_->extent()};
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_->pipeline_layout(),
        0,
        1,
        &descriptor_set,
        0,
        nullptr);

    vkCmdDrawIndexed(command_buffer, count_cast(indices.size()), 2, 0, 0, 0);

    vkCmdBindPipeline(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        ball_pipeline_->pipeline());

    ball_push_consts ball_push_values{
        .resolution = {extent.width, extent.height}};
    ball_push_values.color[0].r = 0.5f;
    ball_push_values.color[1].g = 0.5f;
    ball_push_values.color[2].b = 0.5f;
    ball_push_values.color[3].r = 0.5f;
    ball_push_values.color[4].g = 0.5f;
    ball_push_values.color[5].b = 0.5f;

    vkCmdPushConstants(command_buffer,
        ball_pipeline_->pipeline_layout(),
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(ball_push_consts),
        &ball_push_values);

    vkCmdDrawIndexed(command_buffer, count_cast(indices.size()), 1, 0, 0, 2);

    vkCmdEndRendering(command_buffer);

    transition_image(swap_chain_->image(image_index),
        command_buffer,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error{"unable to end command buffer recording!"};
    }
}

void vkpong::vulkan_renderer::update_uniform_buffer(
    vkpong::vulkan_buffer& buffer)
{
    uniform_buffer_object ubo{};
    ubo.model = glm::mat4{1.0f};

    ubo.view = glm::mat4{1.0f};

    ubo.projection[0][0] = glm::radians(110.f);
    ubo.projection[1][1] = -1;
    ubo.projection[2][2] = 1;
    ubo.projection[3][3] = 1;

    buffer.fill(0, as_bytes(ubo));
}

void vkpong::vulkan_renderer::update_instance_buffer(vkpong::game const& state,
    vkpong::vulkan_buffer& buffer)
{
    std::array data{
        instance_data{.offset = glm::vec2(-.9f, state.player_position),
            .dimension = glm::vec2(0.02f, 0.2f),
            .color = glm::vec3(.5f, 0, 0)},
        instance_data{.offset = glm::vec2(.9f, state.npc_position),
            .dimension = glm::vec2(0.02f, 0.2f),
            .color = glm::vec3(0, .5f, 0)},
        instance_data{.offset = glm::vec2(state.ball_position.first + 0.2f,
                          state.ball_position.second + 0.2f),
            .dimension = glm::vec2(0.2f, 0.2f),
            .color = glm::vec3(0, 0, .5f)}};

    buffer.fill(0, as_bytes(data));
}

bool vkpong::vulkan_renderer::is_multisampled() const
{
    return device_->max_msaa_samples() != VK_SAMPLE_COUNT_1_BIT;
}

void vkpong::vulkan_renderer::recreate_images()
{
    if (is_multisampled())
    {
        cleanup_images();

        // color image
        create_image(device_->physical(),
            device_->logical(),
            swap_chain_->extent(),
            1,
            device_->max_msaa_samples(),
            swap_chain_->image_format(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            color_image_,
            color_image_memory_);

        color_image_view_ = create_image_view(device_->logical(),
            color_image_,
            swap_chain_->image_format(),
            VK_IMAGE_ASPECT_COLOR_BIT,
            1);
    }
}

void vkpong::vulkan_renderer::cleanup_images()
{
    vkDestroyImageView(device_->logical(), color_image_view_, nullptr);
    vkDestroyImage(device_->logical(), color_image_, nullptr);
    vkFreeMemory(device_->logical(), color_image_memory_, nullptr);
}
