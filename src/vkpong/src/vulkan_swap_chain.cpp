#include <vulkan_swap_chain.hpp>

#include <vulkan_context.hpp>
#include <vulkan_device.hpp>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <stdexcept>

namespace
{
    [[nodiscard]] VkSurfaceFormatKHR choose_swap_surface_format(
        std::span<VkSurfaceFormatKHR const> available_formats)
    {
        if (auto const it{std::ranges::find_if(available_formats,
                [](VkSurfaceFormatKHR const& f)
                {
                    return f.format == VK_FORMAT_B8G8R8A8_SRGB &&
                        f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                })};
            it != available_formats.cend())
        {
            return *it;
        }

        return available_formats.front();
    }

    [[nodiscard]] VkPresentModeKHR choose_swap_present_mode(
        std::span<VkPresentModeKHR const> available_present_modes)
    {
        constexpr auto preferred_mode{VK_PRESENT_MODE_MAILBOX_KHR};
        return std::ranges::find(available_present_modes, preferred_mode) !=
                available_present_modes.cend()
            ? preferred_mode
            : VK_PRESENT_MODE_FIFO_KHR;
    }

    [[nodiscard]] VkExtent2D choose_swap_extent(GLFWwindow* const window,
        VkSurfaceCapabilitiesKHR const& capabilities)
    {
        if (capabilities.currentExtent.width !=
            std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }

        int width{};
        int height{};
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actual_extent = {static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)};

        actual_extent.width = std::clamp(actual_extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actual_extent;
    }

    [[nodiscard]] VkImageView create_image_view(VkDevice device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspect_flags,
        uint32_t mip_levels)
    {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;
        view_info.subresourceRange.aspectMask = aspect_flags;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = mip_levels;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VkImageView imageView{};
        if (vkCreateImageView(device, &view_info, nullptr, &imageView) !=
            VK_SUCCESS)
        {
            throw std::runtime_error{"failed to create image view!"};
        }

        return imageView;
    }

    [[nodiscard]] VkSemaphore create_semaphore(
        vkpong::vulkan_device* const device)
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

    [[nodiscard]] VkFence create_fence(vkpong::vulkan_device* const device,
        bool set_signaled)
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
} // namespace

vkpong::swap_chain_support
vkpong::query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    vkpong::swap_chain_support rv;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device,
        surface,
        &rv.capabilities);

    uint32_t format_count{};
    vkGetPhysicalDeviceSurfaceFormatsKHR(device,
        surface,
        &format_count,
        nullptr);
    if (format_count != 0)
    {
        rv.surface_formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device,
            surface,
            &format_count,
            rv.surface_formats.data());
    }

    uint32_t present_count{};
    vkGetPhysicalDeviceSurfacePresentModesKHR(device,
        surface,
        &present_count,
        nullptr);
    if (present_count != 0)
    {
        rv.present_modes.resize(present_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device,
            surface,
            &present_count,
            rv.present_modes.data());
    }

    return rv;
}

vkpong::vulkan_swap_chain::vulkan_swap_chain(GLFWwindow* window,
    vulkan_context* context,
    vulkan_device* device)
    : window_{window}
    , context_{context}
    , device_{device}
    , image_available_{create_semaphore(device)}
    , render_finished_{create_semaphore(device)}
    , in_flight_{create_fence(device, true)}
{
    create_chain_and_images();

    vkGetDeviceQueue(device_->logical,
        device_->graphics_family,
        0,
        &graphics_queue_);

    vkGetDeviceQueue(device_->logical,
        device_->present_family,
        0,
        &present_queue_);

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);
}

vkpong::vulkan_swap_chain::~vulkan_swap_chain()
{
    vkDestroyFence(device_->logical, in_flight_, nullptr);
    vkDestroySemaphore(device_->logical, render_finished_, nullptr);
    vkDestroySemaphore(device_->logical, image_available_, nullptr);

    cleanup();
}

bool vkpong::vulkan_swap_chain::acquire_next_image(uint32_t& image_index)
{
    constexpr auto timeout{std::numeric_limits<uint64_t>::max()};
    vkWaitForFences(device_->logical, 1, &in_flight_, VK_TRUE, UINT64_MAX);

    VkResult result{vkAcquireNextImageKHR(device_->logical,
        chain,
        timeout,
        image_available_,
        VK_NULL_HANDLE,
        &image_index)};
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreate();
        return false;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error{"failed to acquire swap chain image"};
    }

    vkResetFences(device_->logical, 1, &in_flight_);
    return true;
}

void vkpong::vulkan_swap_chain::submit_command_buffer(
    VkCommandBuffer const* const command_buffer,
    uint32_t const image_index)
{
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
    submit_info.pCommandBuffers = command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores.data();

    if (vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    std::array swapchains{chain};
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores.data();
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains.data();
    present_info.pImageIndices = &image_index;

    VkResult result{vkQueuePresentKHR(present_queue_, &present_info)};
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        framebuffer_resized_)
    {
        framebuffer_resized_ = false;
        recreate();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to present swap chain image!"};
    }
}

void vkpong::vulkan_swap_chain::recreate()
{
    int width{};
    int height{};
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window_, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device_->logical);
    cleanup();
    create_chain_and_images();
}

void vkpong::vulkan_swap_chain::framebuffer_resize_callback(GLFWwindow* window,
    [[maybe_unused]] int width,
    [[maybe_unused]] int height)
{
    auto* const swap_chain{
        reinterpret_cast<vulkan_swap_chain*>(glfwGetWindowUserPointer(window))};
    swap_chain->framebuffer_resized_ = true;
}

void vkpong::vulkan_swap_chain::create_chain_and_images()
{
    auto swap_details{
        query_swap_chain_support(device_->physical, context_->surface)};

    VkPresentModeKHR const present_mode{
        choose_swap_present_mode(swap_details.present_modes)};
    VkSurfaceFormatKHR const surface_format{
        choose_swap_surface_format(swap_details.surface_formats)};

    image_format_ = surface_format.format;
    extent_ = choose_swap_extent(window_, swap_details.capabilities);

    uint32_t image_count{swap_details.capabilities.minImageCount + 1};
    if (swap_details.capabilities.maxImageCount > 0)
    {
        image_count =
            std::min(swap_details.capabilities.maxImageCount, image_count);
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = context_->surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent_;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.preTransform = swap_details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    std::array const queue_family_indices{device_->graphics_family,
        device_->present_family};
    if (device_->graphics_family != device_->present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices.data();
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    if (vkCreateSwapchainKHR(device_->logical, &create_info, nullptr, &chain) !=
        VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create swap chain!"};
    }

    if (vkGetSwapchainImagesKHR(device_->logical,
            chain,
            &image_count,
            nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to get swap chain images!"};
    }

    images_.resize(image_count);
    if (vkGetSwapchainImagesKHR(device_->logical,
            chain,
            &image_count,
            images_.data()) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to get swap chain images!"};
    }

    image_views_.resize(image_count);
    for (size_t i{}; i != image_count; ++i)
    {
        image_views_[i] = create_image_view(device_->logical,
            images_[i],
            image_format_,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1);
    }
}

void vkpong::vulkan_swap_chain::cleanup()
{
    for (size_t i{}; i != images_.size(); ++i)
    {
        vkDestroyImageView(device_->logical, image_views_[i], nullptr);
    }

    vkDestroySwapchainKHR(device_->logical, chain, nullptr);
}
