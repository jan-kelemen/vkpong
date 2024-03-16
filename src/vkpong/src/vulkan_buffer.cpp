#include <vulkan_buffer.hpp>

#include <vulkan_device.hpp>
#include <vulkan_utility.hpp>

#include <cassert>
#include <stdexcept>

vkpong::vulkan_buffer::vulkan_buffer(vulkan_device* device,
    VkDeviceSize size,
    VkBufferCreateFlags usage,
    VkMemoryPropertyFlags memory_properties,
    bool keep_mapped)
    : device_{device}
    , size_{size}
    , keep_mapped_{keep_mapped}
{
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size_;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_->logical(), &buffer_info, nullptr, &buffer_) !=
        VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create buffer!"};
    }

    VkMemoryRequirements memory_requirements{};
    vkGetBufferMemoryRequirements(device_->logical(),
        buffer_,
        &memory_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(device_->physical(),
        memory_requirements.memoryTypeBits,
        memory_properties);

    if (vkAllocateMemory(device_->logical(),
            &alloc_info,
            nullptr,
            &device_memory_) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to allocate buffer memory!"};
    }

    vkBindBufferMemory(device_->logical(), buffer_, device_memory_, 0);

    if (keep_mapped_)
    {
        map_memory(0, size_);
    }
}

vkpong::vulkan_buffer::vulkan_buffer(vulkan_buffer&& other) noexcept
    : device_{std::exchange(other.device_, nullptr)}
    , size_{std::exchange(other.size_, {})}
    , buffer_{std::exchange(other.buffer_, nullptr)}
    , device_memory_{std::exchange(other.device_memory_, nullptr)}
    , keep_mapped_{std::exchange(other.keep_mapped_, {})}
    , memory_mapping_{std::exchange(other.memory_mapping_, {})}
{
}

vkpong::vulkan_buffer::~vulkan_buffer()
{
    if (device_)
    {
        if (memory_mapping_)
        {
            unmap_memory();
        }

        vkDestroyBuffer(device_->logical(), buffer_, nullptr);
        vkFreeMemory(device_->logical(), device_memory_, nullptr);
    }
}

void vkpong::vulkan_buffer::fill(size_t const offset,
    std::span<std::byte const> bytes)
{
    size_t offset_in_mapping{offset};
    if (!keep_mapped_)
    {
        map_memory(offset, bytes.size());
        offset_in_mapping = 0;
    }

    memcpy(reinterpret_cast<std::byte*>(memory_mapping_) + offset_in_mapping,
        bytes.data(),
        bytes.size());

    if (!keep_mapped_)
    {
        unmap_memory();
    }
}

vkpong::vulkan_buffer& vkpong::vulkan_buffer::operator=(
    vulkan_buffer&& other) noexcept
{
    using std::swap;

    if (this != &other)
    {
        swap(device_, other.device_);
        swap(size_, other.size_);
        swap(buffer_, other.buffer_);
        swap(device_memory_, other.device_memory_);
        swap(keep_mapped_, other.keep_mapped_);
        swap(memory_mapping_, other.memory_mapping_);
    }

    return *this;
}

void vkpong::vulkan_buffer::map_memory(size_t const offset, size_t const size)
{
    assert(size + offset <= size_);

    if (vkMapMemory(device_->logical(),
            device_memory_,
            offset,
            size,
            0,
            &memory_mapping_) != VK_SUCCESS)
    {
        throw std::runtime_error{"unable to map memory!"};
    }
}

void vkpong::vulkan_buffer::unmap_memory()
{
    vkUnmapMemory(device_->logical(), device_memory_);
    memory_mapping_ = nullptr;
}
