#ifndef VKPONG_VULKAN_UTILITY_INCLUDED
#define VKPONG_VULKAN_UTILITY_INCLUDED

#include <vulkan/vulkan_core.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace vkpong
{
    [[nodiscard]] uint32_t find_memory_type(VkPhysicalDevice physical_device,
        uint32_t type_filter,
        VkMemoryPropertyFlags properties);

    template<typename T>
    [[nodiscard]] constexpr uint32_t count_cast(T const count)
    {
        assert(std::in_range<uint32_t>(count));
        return static_cast<uint32_t>(count);
    }

    template<typename T>
    std::span<std::byte const> as_bytes(T const& value,
        size_t const size = sizeof(T))
    {
        // NOLINTNEXTLINE
        return {reinterpret_cast<std::byte const*>(&value), size};
    }

    template<typename T>
    std::span<std::byte const> as_bytes(std::vector<T> const& value,
        std::optional<size_t> const elements = std::nullopt)
    {
        // NOLINTNEXTLINE
        return {reinterpret_cast<std::byte const*>(value.data()),
            elements.value_or(value.size()) * sizeof(T)};
    }

    void create_image(VkPhysicalDevice physical_device,
        VkDevice device,
        VkExtent2D extent,
        uint32_t mip_levels,
        VkSampleCountFlagBits samples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& image_memory);

    [[nodiscard]] VkImageView create_image_view(VkDevice device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspect_flags,
        uint32_t mip_levels);
} // namespace vkpong

#endif // !VKPONG_VULKAN_UTILITY_INCLUDED
