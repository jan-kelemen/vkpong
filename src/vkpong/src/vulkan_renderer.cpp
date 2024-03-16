#include <vulkan_renderer.hpp>

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
#include <chrono>
#include <limits>
#include <span>
#include <stdexcept>

namespace
{
    struct [[nodiscard]] push_consts final
    {
        glm::fvec4 color[6];
    };

    struct [[nodiscard]] instance_data final
    {
        glm::fvec2 offset;
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
                    .offset = offsetof(instance_data, offset)}};

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

namespace
{
    [[nodiscard]] VkCommandPool create_command_pool(
        vkpong::vulkan_device* const device)
    {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        pool_info.queueFamilyIndex = device->graphics_family();

        VkCommandPool rv;
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

    std::pair<VkBuffer, VkDeviceMemory> create_vertex_and_index_buffer(
        vkpong::vulkan_device* const device)
    {
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = sizeof(vertices[0]) * vertices.size() +
            sizeof(indices[0]) * indices.size();
        buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        return vkpong::create_buffer(device->physical(),
            device->logical(),
            buffer_info.size,
            buffer_info.usage,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    std::pair<VkBuffer, VkDeviceMemory> create_instance_buffer(
        vkpong::vulkan_device* const device)
    {
        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = sizeof(instance_data) * 2;
        buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        return vkpong::create_buffer(device->physical(),
            device->logical(),
            buffer_info.size,
            buffer_info.usage,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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

        VkDescriptorSetLayout rv;
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

} // namespace

vkpong::vulkan_renderer::vulkan_renderer(
    std::unique_ptr<vulkan_context> context,
    std::unique_ptr<vulkan_device> device,
    std::unique_ptr<vulkan_swap_chain> swap_chain)
    : context_{std::move(context)}
    , device_{std::move(device)}
    , swap_chain_{std::move(swap_chain)}
    , command_pool_{create_command_pool(device_.get())}
    , command_buffers_{vulkan_swap_chain::max_frames_in_flight}
    , descriptor_set_layout_{create_descriptor_set_layout(device_.get())}
    , descriptor_pool_{create_descriptor_pool(device_.get())}
{
    vulkan_pipeline_builder pipeline_builder{device_.get(),
        swap_chain_->image_format()};

    pipeline_builder.add_shader(VK_SHADER_STAGE_VERTEX_BIT, "vert.spv", "main");
    pipeline_builder.add_shader(VK_SHADER_STAGE_FRAGMENT_BIT,
        "frag.spv",
        "main");
    pipeline_builder.with_rasterization_samples(device_->max_msaa_samples());
    pipeline_builder.add_vertex_input(vertex::binding_description(),
        vertex::attribute_descriptions());
    pipeline_builder.with_push_constants(
        VkPushConstantRange{.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(push_consts)});
    pipeline_builder.add_descriptor_set_layout(descriptor_set_layout_);

    pipeline_ = std::make_unique<vulkan_pipeline>(pipeline_builder.build());

    create_command_buffers(device_.get(),
        command_pool_,
        vulkan_swap_chain::max_frames_in_flight,
        command_buffers_);

    std::tie(vertex_and_index_buffer_, vertex_and_index_memory_) =
        create_vertex_and_index_buffer(device_.get());

    descriptor_sets_.resize(vulkan_swap_chain::max_frames_in_flight);
    create_descriptor_sets(device_.get(),
        descriptor_set_layout_,
        descriptor_pool_,
        descriptor_sets_);

    for (int i{}; i != vulkan_swap_chain::max_frames_in_flight; ++i)
    {
        instance_buffers_.emplace_back(device_.get(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            sizeof(instance_data) * 2);

        auto const& buffer{uniform_buffers_.emplace_back(device_.get(),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            sizeof(uniform_buffer_object))};
        bind_descriptor_set(device_.get(), descriptor_sets_[i], buffer.buffer);
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

    vkDestroyBuffer(device_->logical(), vertex_and_index_buffer_, nullptr);
    vkFreeMemory(device_->logical(), vertex_and_index_memory_, nullptr);

    vkDestroyCommandPool(device_->logical(), command_pool_, nullptr);
}

void vkpong::vulkan_renderer::draw()
{
    uint32_t image_index{};
    if (!swap_chain_->acquire_next_image(current_frame_, image_index))
    {
        return;
    }

    auto& command_buffer{command_buffers_[current_frame_]};
    auto const& descriptor_set{descriptor_sets_[current_frame_]};

    vkResetCommandBuffer(command_buffer, 0);

    record_command_buffer(command_buffer, descriptor_set, image_index);

    update_uniform_buffer(uniform_buffers_[current_frame_]);
    update_instance_buffer(instance_buffers_[current_frame_]);

    swap_chain_->submit_command_buffer(&command_buffer,
        current_frame_,
        image_index);
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

    VkImageMemoryBarrier bimage_memory_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .image = swap_chain_->image(image_index),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }};

    vkCmdPipelineBarrier(command_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &bimage_memory_barrier);

    constexpr VkClearValue clear_value{{{0.0f, 4.0f, 0.0f, 1.0f}}};
    VkRenderingAttachmentInfoKHR color_attachment_info{};
    color_attachment_info.sType =
        VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    color_attachment_info.imageLayout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_info.clearValue = clear_value;
    if (swap_chain_->is_multisampled())
    {
        color_attachment_info.imageView =
            swap_chain_->intermediate_image_view();
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

    VkRenderingInfoKHR const render_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = {{0, 0}, swap_chain_->extent()},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
    };

    vkCmdBeginRendering(command_buffer, &render_info);

    vkCmdBindPipeline(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_->pipeline());

    void* data{};
    if (vkMapMemory(device_->logical(),
            vertex_and_index_memory_,
            0,
            sizeof(vertices[0]) * vertices.size(),
            0,
            &data) != VK_SUCCESS)
    {
        throw std::runtime_error{"unable to map buffer memory!"};
    }
    size_t const vertices_size{sizeof(vertices[0]) * vertices.size()};
    memcpy(data, vertices.data(), vertices_size);

    size_t const indices_size{sizeof(indices[0]) * indices.size()};
    memcpy(reinterpret_cast<std::byte*>(data) + vertices_size,
        indices.data(),
        indices_size);
    vkUnmapMemory(device_->logical(), vertex_and_index_memory_);

    std::array vertex_buffer{vertex_and_index_buffer_};
    std::array instance_buffer{instance_buffers_[current_frame_].buffer};
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
        vertex_and_index_buffer_,
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

    push_consts push_values{};
    push_values.color[0].r = 0.5f;
    push_values.color[1].g = 0.5f;
    push_values.color[2].b = 0.5f;
    push_values.color[3].r = 0.5f;
    push_values.color[4].g = 0.5f;
    push_values.color[5].b = 0.5f;

    vkCmdPushConstants(command_buffer,
        pipeline_->pipeline_layout(),
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(push_consts),
        &push_values);

    vkCmdBindDescriptorSets(command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_->pipeline_layout(),
        0,
        1,
        &descriptor_set,
        0,
        nullptr);

    vkCmdDrawIndexed(command_buffer, count_cast(indices.size()), 2, 0, 0, 0);

    vkCmdEndRendering(command_buffer);

    VkImageMemoryBarrier const image_memory_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = swap_chain_->image(image_index),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }};

    vkCmdPipelineBarrier(command_buffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &image_memory_barrier);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
    {
        throw std::runtime_error{"unable to end command buffer recording!"};
    }
}

void vkpong::vulkan_renderer::update_uniform_buffer(mapped_buffer& buffer)
{
    uniform_buffer_object ubo{};
    ubo.model = glm::translate(glm::mat4{1.0f}, {.9f, .0f, .0f});

    ubo.view = glm::mat4{1.0f};

    ubo.projection[0][0] = .05f;
    ubo.projection[1][1] = -1;
    ubo.projection[2][2] = 1;
    ubo.projection[3][3] = 3;

    memcpy(buffer.mapped_memory, &ubo, sizeof(uniform_buffer_object));
}

void vkpong::vulkan_renderer::update_instance_buffer(mapped_buffer& buffer)
{
    std::array data{instance_data{.offset = glm::vec2(-50.f, .0f)},
        instance_data{.offset = glm::vec2(50.f, .0f)}};

    memcpy(buffer.mapped_memory, data.data(), sizeof(data));
}

vkpong::vulkan_renderer::mapped_buffer::mapped_buffer(vulkan_device* device,
    VkBufferUsageFlags usage,
    size_t size)
    : device_{device}
{
    std::tie(buffer, device_memory) = create_buffer(device_->physical(),
        device_->logical(),
        size,
        usage,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkMapMemory(device_->logical(),
            device_memory,
            0,
            size,
            0,
            &mapped_memory) != VK_SUCCESS)
    {
        throw std::runtime_error{"unable to map memory!"};
    }
}

vkpong::vulkan_renderer::mapped_buffer::mapped_buffer(
    mapped_buffer&& other) noexcept
    : device_{std::exchange(other.device_, nullptr)}
    , buffer{std::exchange(other.buffer, {})}
    , device_memory{std::exchange(other.device_memory, {})}
    , mapped_memory{std::exchange(other.mapped_memory, {})}
{
}

vkpong::vulkan_renderer::mapped_buffer::~mapped_buffer()
{
    if (device_memory != nullptr)
    {
        vkUnmapMemory(device_->logical(), device_memory);
        vkDestroyBuffer(device_->logical(), buffer, nullptr);
        vkFreeMemory(device_->logical(), device_memory, nullptr);
    }
}
