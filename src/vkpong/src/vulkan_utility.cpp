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

std::pair<VkBuffer, VkDeviceMemory> vkpong::create_buffer(
    VkPhysicalDevice const physical_device,
    VkDevice const device,
    VkDeviceSize const size,
    VkBufferUsageFlags const usage,
    VkMemoryPropertyFlags const properties)
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer;
    if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create buffer!"};
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(physical_device,
        memory_requirements.memoryTypeBits,
        properties);

    VkDeviceMemory buffer_memory;
    if (vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory) !=
        VK_SUCCESS)
    {
        // todo-jk: handle this properly to avoid leaks
        vkDestroyBuffer(device, buffer, nullptr);
        throw std::runtime_error{"failed to allocate buffer memory!"};
    }

    vkBindBufferMemory(device, buffer, buffer_memory, 0);

    return {buffer, buffer_memory};
}
