#include <vulkan_renderer.hpp>

#include <vulkan_context.hpp>
#include <vulkan_device.hpp>
#include <vulkan_pipeline.hpp>
#include <vulkan_swap_chain.hpp>

#include <array>
#include <limits>
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

    VkSemaphore create_semaphore(vkpong::vulkan_device* const device)
    {
        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore rv;
        if (vkCreateSemaphore(device->logical, &semaphore_info, nullptr, &rv) !=
            VK_SUCCESS)
        {
            throw std::runtime_error{"failed to create semaphore"};
        }

        return rv;
    }

    VkFence create_fence(vkpong::vulkan_device* const device, bool set_signaled)
    {
        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (set_signaled)
        {
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        VkFence rv;
        if (vkCreateFence(device->logical, &fence_info, nullptr, &rv) !=
            VK_SUCCESS)
        {
            throw std::runtime_error{"failed to create fence"};
        }

        return rv;
    }

    VkCommandBuffer create_command_buffer(vkpong::vulkan_device* const device,
        VkCommandPool command_pool)
    {
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        VkCommandBuffer rv;
        if (vkAllocateCommandBuffers(device->logical, &alloc_info, &rv) !=
            VK_SUCCESS)
        {
            throw std::runtime_error{"failed to allocate command buffer!"};
        }

        return rv;
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
    , command_buffer_{create_command_buffer(device_.get(), command_pool_)}
    , image_available_{create_semaphore(device_.get())}
    , render_finished_{create_semaphore(device_.get())}
    , in_flight_{create_fence(device_.get(), true)}
{
    vkGetDeviceQueue(device_->logical,
        device_->graphics_family,
        0,
        &graphics_queue_);

    vkGetDeviceQueue(device_->logical,
        device_->present_family,
        0,
        &present_queue_);
}

vkpong::vulkan_renderer::~vulkan_renderer()
{
    vkDestroyFence(device_->logical, in_flight_, nullptr);
    vkDestroySemaphore(device_->logical, render_finished_, nullptr);
    vkDestroySemaphore(device_->logical, image_available_, nullptr);
    vkDestroyCommandPool(device_->logical, command_pool_, nullptr);
}

void vkpong::vulkan_renderer::draw()
{
    constexpr auto timeout{std::numeric_limits<uint64_t>::max()};
    vkWaitForFences(device_->logical, 1, &in_flight_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_->logical, 1, &in_flight_);

    uint32_t image_index{};
    vkAcquireNextImageKHR(device_->logical,
        swap_chain_->chain,
        timeout,
        image_available_,
        VK_NULL_HANDLE,
        &image_index);

    vkResetCommandBuffer(command_buffer_, 0);
    record_command_buffer(image_index);

    std::array const wait_semaphores{image_available_};
    std::array<VkPipelineStageFlags, 1> const wait_stages{
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::array const signal_semaphores{render_finished_};

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.pWaitDstStageMask = wait_stages.data();
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer_;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores.data();

    if (vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    std::array swapchains{swap_chain_->chain};
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores.data();
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains.data();
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(present_queue_, &present_info);
}

void vkpong::vulkan_renderer::record_command_buffer(uint32_t const image_index)
{
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(command_buffer_, &begin_info);

    VkImageMemoryBarrier const bimage_memory_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .image = swap_chain_->images_[image_index],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }};

    vkCmdPipelineBarrier(command_buffer_,
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
    VkRenderingAttachmentInfoKHR const color_attachment_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = swap_chain_->image_views_[image_index],
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clear_value,
    };

    VkRenderingInfoKHR const render_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .renderArea = {{0, 0}, swap_chain_->extent_},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
    };

    vkCmdBeginRendering(command_buffer_, &render_info);

    vkCmdBindPipeline(command_buffer_,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_->pipeline_);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swap_chain_->extent_.width);
    viewport.height = static_cast<float>(swap_chain_->extent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer_, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_->extent_;
    vkCmdSetScissor(command_buffer_, 0, 1, &scissor);

    vkCmdDraw(command_buffer_, 3, 1, 0, 0);

    vkCmdEndRendering(command_buffer_);

    VkImageMemoryBarrier const image_memory_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = swap_chain_->images_[image_index],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }};

    vkCmdPipelineBarrier(command_buffer_,
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

    vkEndCommandBuffer(command_buffer_);
}
