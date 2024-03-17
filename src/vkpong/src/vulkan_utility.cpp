#include <vulkan_utility.hpp>

#include <stdexcept>

uint32_t vkpong::find_memory_type(VkPhysicalDevice const physical_device,
    uint32_t const type_filter,
    VkMemoryPropertyFlags const properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1 << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & properties) ==
                properties)
        {
            return i;
        }
    }

    throw std::runtime_error{"failed to find suitable memory type!"};
}

void vkpong::create_image(VkPhysicalDevice physical_device,
    VkDevice device,
    VkExtent2D extent,
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
    image_info.extent.width = extent.width;
    image_info.extent.height = extent.height;
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
        vkDestroyImage(device, image, nullptr);
        throw std::runtime_error{"failed to allocate image memory!"};
    }

    if (vkBindImageMemory(device, image, image_memory, 0) != VK_SUCCESS)
    {
        vkFreeMemory(device, image_memory, nullptr);
        vkDestroyImage(device, image, nullptr);
        throw std::runtime_error{"failed to bind image memory!"};
    };
}

[[nodiscard]]
VkImageView vkpong::create_image_view(VkDevice device,
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
