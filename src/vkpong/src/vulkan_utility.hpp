#ifndef VKPONG_VULKAN_UTILITY_INCLUDED
#define VKPONG_VULKAN_UTILITY_INCLUDED

#include <vulkan/vulkan_core.h>

#include <utility>

namespace vkpong
{
    [[nodiscard]] uint32_t find_memory_type(VkPhysicalDevice physical_device,
        uint32_t type_filter,
        VkMemoryPropertyFlags properties);

    [[nodiscard]] std::pair<VkBuffer, VkDeviceMemory> create_buffer(
        VkPhysicalDevice physical_device,
        VkDevice device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);
} // namespace vkpong

#endif // !VKPONG_VULKAN_UTILITY_INCLUDED
