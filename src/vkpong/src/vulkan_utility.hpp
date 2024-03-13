#ifndef VKPONG_VULKAN_UTILITY_INCLUDED
#define VKPONG_VULKAN_UTILITY_INCLUDED

#include <vulkan/vulkan_core.h>

#include <cassert>
#include <utility>

namespace vkpong
{
    template<typename T>
    [[nodiscard]] constexpr uint32_t count_cast(T count)
    {
        assert(std::in_range<uint32_t>(count));
        return static_cast<uint32_t>(count);
    }

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
