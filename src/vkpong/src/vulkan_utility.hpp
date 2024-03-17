#ifndef VKPONG_VULKAN_UTILITY_INCLUDED
#define VKPONG_VULKAN_UTILITY_INCLUDED

#include <vulkan/vulkan_core.h>

#include <cassert>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace vkpong
{
    template<typename T>
    [[nodiscard]] constexpr uint32_t count_cast(T const count)
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

    template<typename T>
    std::span<std::byte const> as_bytes(T const& value,
        size_t const size = sizeof(T))
    {
        return {reinterpret_cast<std::byte const*>(&value), size};
    }

    template<typename T>
    std::span<std::byte const> as_bytes(std::vector<T> const& value,
        std::optional<size_t> const elements = std::nullopt)
    {
        return {reinterpret_cast<std::byte const*>(value.data()),
            elements.value_or(value.size()) * sizeof(T)};
    }
} // namespace vkpong

#endif // !VKPONG_VULKAN_UTILITY_INCLUDED
