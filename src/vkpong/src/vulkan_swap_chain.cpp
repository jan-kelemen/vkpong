#include <vulkan_swap_chain.hpp>

#include <vulkan_context.hpp>
#include <vulkan_device.hpp>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
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

} // namespace

vkpong::swap_chain::~swap_chain()
{
    for (size_t i{}; i != images_.size(); ++i)
    {
        vkDestroyImageView(device_->logical, image_views_[i], nullptr);
    }

    vkDestroySwapchainKHR(device_->logical, chain, nullptr);
}

std::unique_ptr<vkpong::swap_chain> vkpong::create_swap_chain(
    GLFWwindow* const window,
    vulkan_context* const context,
    vulkan_device* const device)
{
    auto const& swap_details{device->swap_chain_details};

    VkPresentModeKHR const present_mode{
        choose_swap_present_mode(swap_details.present_modes)};
    VkSurfaceFormatKHR const surface_format{
        choose_swap_surface_format(swap_details.surface_formats)};

    auto rv{std::make_unique<swap_chain>()};
    rv->image_format_ = surface_format.format;
    rv->extent_ = choose_swap_extent(window, swap_details.capabilities);

    uint32_t image_count{swap_details.capabilities.minImageCount + 1};
    if (swap_details.capabilities.maxImageCount > 0)
    {
        image_count =
            std::min(swap_details.capabilities.maxImageCount, image_count);
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = context->surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = rv->extent_;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.preTransform = swap_details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    std::array const queue_family_indices{device->graphics_family,
        device->present_family};
    if (device->graphics_family != device->present_family)
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

    if (vkCreateSwapchainKHR(device->logical,
            &create_info,
            nullptr,
            &rv->chain) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create swap chain!"};
    }
    rv->device_ = device;

    if (vkGetSwapchainImagesKHR(device->logical,
            rv->chain,
            &image_count,
            nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to get swap chain images!"};
    }
    rv->images_.resize(image_count);
    if (vkGetSwapchainImagesKHR(device->logical,
            rv->chain,
            &image_count,
            rv->images_.data()) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to get swap chain images!"};
    }

    rv->image_views_.resize(image_count);
    for (size_t i{}; i != image_count; ++i)
    {
        rv->image_views_[i] = create_image_view(device->logical,
            rv->images_[i],
            rv->image_format_,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1);
    }

    return rv;
}
