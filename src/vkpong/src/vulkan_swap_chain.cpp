#include <vulkan_swap_chain.hpp>

#include <vulkan_context.hpp>
#include <vulkan_device.hpp>
#include <vulkan_utility.hpp>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <stdexcept>

namespace
{
    void create_image(VkPhysicalDevice physical_device,
        VkDevice device,
        uint32_t width,
        uint32_t height,
        uint32_t mip_levels,
        VkSampleCountFlagBits samples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& image_memory)
    {
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = mip_levels;
        image_info.arrayLayers = 1;
        image_info.format = format;
        image_info.tiling = tiling;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = usage;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.samples = samples;
        image_info.flags = 0;

        if (vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error{"failed to create image!"};
        }

        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device, image, &memory_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = memory_requirements.size;
        alloc_info.memoryTypeIndex = vkpong::find_memory_type(physical_device,
            memory_requirements.memoryTypeBits,
            properties);
        if (vkAllocateMemory(device, &alloc_info, nullptr, &image_memory) !=
            VK_SUCCESS)
        {
            throw std::runtime_error{"failed to allocate image memory!"};
        }
        vkBindImageMemory(device, image, image_memory, 0);
    }
} // namespace

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
        if (vkCreateSemaphore(device->logical(),
                &semaphore_info,
                nullptr,
                &rv) != VK_SUCCESS)
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
        if (vkCreateFence(device->logical(), &fence_info, nullptr, &rv) !=
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
{
    create_chain_and_images();
    for (int i{}; i != max_frames_in_flight; ++i)
    {
        image_syncs_.emplace_back(device_);
    }

    vkGetDeviceQueue(device_->logical(),
        device_->graphics_family(),
        0,
        &graphics_queue_);

    vkGetDeviceQueue(device_->logical(),
        device_->present_family(),
        0,
        &present_queue_);

    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);
}

vkpong::vulkan_swap_chain::vulkan_swap_chain(vulkan_swap_chain&& other) noexcept
    : window_{other.window_}
    , context_{other.context_}
    , device_{std::exchange(other.device_, nullptr)}
    , image_format_{other.image_format_}
    , extent_{other.extent_}
    , chain{std::exchange(other.chain, nullptr)}
    , images_{std::move(other.images_)}
    , image_views_{std::move(other.image_views_)}
    , image_syncs_{std::move(other.image_syncs_)}
    , color_image_{std::exchange(other.color_image_, nullptr)}
    , color_image_memory_{std::exchange(other.color_image_memory_, nullptr)}
    , color_image_view_{std::exchange(other.color_image_view_, nullptr)}
    , graphics_queue_{other.graphics_queue_}
    , present_queue_{other.present_queue_}
{
}

vkpong::vulkan_swap_chain::~vulkan_swap_chain()
{
    if (device_)
    {
        image_syncs_.clear();
        cleanup();
    }
}

bool vkpong::vulkan_swap_chain::is_multisampled() const noexcept
{
    return device_->max_msaa_samples() != VK_SAMPLE_COUNT_1_BIT;
}

bool vkpong::vulkan_swap_chain::acquire_next_image(uint32_t const current_frame,
    uint32_t& image_index)
{
    constexpr auto timeout{std::numeric_limits<uint64_t>::max()};

    auto const& sync{image_syncs_[current_frame]};

    vkWaitForFences(device_->logical(),
        1,
        &sync.in_flight,
        VK_TRUE,
        UINT64_MAX);

    VkResult result{vkAcquireNextImageKHR(device_->logical(),
        chain,
        timeout,
        sync.image_available,
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

    vkResetFences(device_->logical(), 1, &sync.in_flight);
    return true;
}

void vkpong::vulkan_swap_chain::submit_command_buffer(
    VkCommandBuffer const* const command_buffer,
    uint32_t const current_frame,
    uint32_t const image_index)
{
    auto const& sync{image_syncs_[current_frame]};

    std::array const wait_semaphores{sync.image_available};
    std::array<VkPipelineStageFlags, 1> const wait_stages{
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    std::array const signal_semaphores{sync.render_finished};

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.pWaitDstStageMask = wait_stages.data();
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores.data();

    if (vkQueueSubmit(graphics_queue_, 1, &submit_info, sync.in_flight) !=
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

    vkDeviceWaitIdle(device_->logical());
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

vkpong::vulkan_swap_chain& vkpong::vulkan_swap_chain::operator=(
    vulkan_swap_chain&& other) noexcept
{
    using std::swap;

    if (this != &other)
    {
        swap(window_, other.window_);
        swap(context_, other.context_);
        swap(device_, other.device_);
        swap(image_format_, other.image_format_);
        swap(extent_, other.extent_);
        swap(chain, other.chain);
        swap(images_, other.images_);
        swap(image_views_, other.image_views_);
        swap(color_image_, other.color_image_);
        swap(color_image_memory_, other.color_image_memory_);
        swap(color_image_view_, other.color_image_view_);
        swap(graphics_queue_, other.graphics_queue_);
        swap(present_queue_, other.present_queue_);
    }

    return *this;
}

void vkpong::vulkan_swap_chain::create_chain_and_images()
{
    auto swap_details{
        query_swap_chain_support(device_->physical(), context_->surface())};

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
    create_info.surface = context_->surface();
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

    std::array const queue_family_indices{device_->graphics_family(),
        device_->present_family()};
    if (device_->graphics_family() != device_->present_family())
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

    if (vkCreateSwapchainKHR(device_->logical(),
            &create_info,
            nullptr,
            &chain) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create swap chain!"};
    }

    if (vkGetSwapchainImagesKHR(device_->logical(),
            chain,
            &image_count,
            nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to get swap chain images!"};
    }

    images_.resize(image_count);
    if (vkGetSwapchainImagesKHR(device_->logical(),
            chain,
            &image_count,
            images_.data()) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to get swap chain images!"};
    }

    image_views_.resize(image_count);
    for (size_t i{}; i != image_count; ++i)
    {
        image_views_[i] = create_image_view(device_->logical(),
            images_[i],
            image_format_,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1);
    }

    if (is_multisampled())
    {
        // color image
        create_image(device_->physical(),
            device_->logical(),
            extent_.width,
            extent_.height,
            1,
            device_->max_msaa_samples(),
            image_format_,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            color_image_,
            color_image_memory_);
        color_image_view_ = create_image_view(device_->logical(),
            color_image_,
            image_format_,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1);
    }
}

void vkpong::vulkan_swap_chain::cleanup()
{
    if (is_multisampled())
    {
        vkDestroyImageView(device_->logical(), color_image_view_, nullptr);
        vkDestroyImage(device_->logical(), color_image_, nullptr);
        vkFreeMemory(device_->logical(), color_image_memory_, nullptr);
    }

    for (size_t i{}; i != images_.size(); ++i)
    {
        vkDestroyImageView(device_->logical(), image_views_[i], nullptr);
    }

    vkDestroySwapchainKHR(device_->logical(), chain, nullptr);
}

vkpong::vulkan_swap_chain::image_sync::image_sync(
    vkpong::vulkan_device* const device)
    : device_{device}
{
    image_available = create_semaphore(device);
    render_finished = create_semaphore(device);
    in_flight = create_fence(device, true);
}

vkpong::vulkan_swap_chain::image_sync::image_sync(image_sync&& other) noexcept
    : device_{other.device_}
    , image_available{other.image_available}
    , render_finished{other.render_finished}
    , in_flight{other.in_flight}
{
    other.image_available = {};
    other.render_finished = {};
    other.in_flight = {};
}

vkpong::vulkan_swap_chain::image_sync::~image_sync()
{
    vkDestroyFence(device_->logical(), in_flight, nullptr);
    vkDestroySemaphore(device_->logical(), render_finished, nullptr);
    vkDestroySemaphore(device_->logical(), image_available, nullptr);
}
