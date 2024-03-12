#include <vulkan_renderer.hpp>

#include <vulkan_context.hpp>
#include <vulkan_device.hpp>
#include <vulkan_pipeline.hpp>
#include <vulkan_swap_chain.hpp>

#include <array>
#include <cassert>
#include <limits>
#include <span>
#include <stdexcept>

namespace
{
    [[nodiscard]] VkCommandPool create_command_pool(
        vkpong::vulkan_device* const device)
    {
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        pool_info.queueFamilyIndex = device->graphics_family;

        VkCommandPool rv;
        if (vkCreateCommandPool(device->logical, &pool_info, nullptr, &rv) !=
            VK_SUCCESS)
        {
            throw std::runtime_error{"failed to create command pool"};
        }

        return rv;
    }

    void create_command_buffers(vkpong::vulkan_device* const device,
        VkCommandPool command_pool,
        uint32_t count,
        std::span<VkCommandBuffer> data_buffer)
    {
        assert(data_buffer.size() >= count);

        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = count;

        if (vkAllocateCommandBuffers(device->logical,
                &alloc_info,
                data_buffer.data()) != VK_SUCCESS)
        {
            throw std::runtime_error{"failed to allocate command buffers!"};
        }
    }
} // namespace

vkpong::vulkan_renderer::vulkan_renderer(
    std::unique_ptr<vulkan_context> context,
    std::unique_ptr<vulkan_device> device,
    std::unique_ptr<vulkan_swap_chain> swap_chain,
    std::unique_ptr<vulkan_pipeline> pipeline)
    : context_{std::move(context)}
    , device_{std::move(device)}
    , swap_chain_{std::move(swap_chain)}
    , pipeline_{std::move(pipeline)}
    , command_pool_{create_command_pool(device_.get())}
    , command_buffers_{vulkan_swap_chain::max_frames_in_flight}
{
    create_command_buffers(device_.get(),
        command_pool_,
        vulkan_swap_chain::max_frames_in_flight,
        command_buffers_);
}

vkpong::vulkan_renderer::~vulkan_renderer()
{
    vkDeviceWaitIdle(device_->logical);

    vkDestroyCommandPool(device_->logical, command_pool_, nullptr);
}

void vkpong::vulkan_renderer::draw()
{
    uint32_t image_index{};
    if (!swap_chain_->acquire_next_image(current_frame_, image_index))
    {
        return;
    }

    vkResetCommandBuffer(command_buffers_[current_frame_], 0);
    record_command_buffer(image_index);

    swap_chain_->submit_command_buffer(&command_buffers_[current_frame_],
        current_frame_,
        image_index);
    current_frame_ =
        (current_frame_ + 1) % vulkan_swap_chain::max_frames_in_flight;
}

void vkpong::vulkan_renderer::record_command_buffer(uint32_t const image_index)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(command_buffers_[current_frame_], &begin_info);

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

    vkCmdPipelineBarrier(command_buffers_[current_frame_],
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // srcStageMask
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
        0,
        0,
        nullptr,
        0,
        nullptr,
        1, // imageMemoryBarrierCount
        &bimage_memory_barrier // pImageMemoryBarriers
    );

    constexpr VkClearValue clear_value{{{0.0f, 4.0f, 0.0f, 1.0f}}};
    VkRenderingAttachmentInfoKHR color_attachment_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clear_value};
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

    vkCmdBeginRendering(command_buffers_[current_frame_], &render_info);

    vkCmdBindPipeline(command_buffers_[current_frame_],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_->pipeline());

    VkExtent2D const extent{swap_chain_->extent()};
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffers_[current_frame_], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(command_buffers_[current_frame_], 0, 1, &scissor);

    push_consts push_values{};
    push_values.color[0].r = 0.5f;
    push_values.color[1].g = 0.5f;
    push_values.color[2].b = 0.5f;

    vkCmdPushConstants(command_buffers_[current_frame_],
        pipeline_->layout(),
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(push_consts),
        &push_values);

    vkCmdDraw(command_buffers_[current_frame_], 3, 1, 0, 0);

    vkCmdEndRendering(command_buffers_[current_frame_]);

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

    vkCmdPipelineBarrier(command_buffers_[current_frame_],
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // dstStageMask
        0,
        0,
        nullptr,
        0,
        nullptr,
        1, // imageMemoryBarrierCount
        &image_memory_barrier // pImageMemoryBarriers
    );

    vkEndCommandBuffer(command_buffers_[current_frame_]);
}
