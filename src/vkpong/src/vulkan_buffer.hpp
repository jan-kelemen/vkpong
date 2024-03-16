#ifndef VKPONG_VULKAN_BUFFER_INCLUDED
#define VKPONG_VULKAN_BUFFER_INCLUDED

#include <vulkan/vulkan_core.h>

#include <memory>
#include <optional>
#include <span>

namespace vkpong
{
    class vulkan_device;
} // namespace vkpong

namespace vkpong
{
    class [[nodiscard]] vulkan_buffer final
    {
    public: // Construction
        vulkan_buffer(vulkan_device* device,
            VkDeviceSize buffer_size,
            VkBufferCreateFlags usage,
            VkMemoryPropertyFlags memory_properties,
            bool keep_mapped = false);

        vulkan_buffer(vulkan_buffer const&) = delete;

        vulkan_buffer(vulkan_buffer&& other) noexcept;

    public: // Destruction
        ~vulkan_buffer();

    public: // Interface
        [[nodiscard]] constexpr VkBuffer buffer() const noexcept;

        void fill(size_t offset, std::span<std::byte const> bytes);

    public: // Operators
        vulkan_buffer& operator=(vulkan_buffer const&) = delete;

        vulkan_buffer& operator=(vulkan_buffer&& other) noexcept;

    private: // Helpers
        void map_memory(size_t offset, size_t size);
        void unmap_memory();

    private: // Data
        vulkan_device* device_;
        VkDeviceSize size_{};
        VkBuffer buffer_{};
        VkDeviceMemory device_memory_{};
        bool keep_mapped_{};
        void* memory_mapping_{};
    };
} // namespace vkpong

inline constexpr VkBuffer vkpong::vulkan_buffer::buffer() const noexcept
{
    return buffer_;
}

#endif // !VKPONG_VULKAN_BUFFER_INCLUDED
