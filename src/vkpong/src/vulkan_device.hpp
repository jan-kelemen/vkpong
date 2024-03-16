#ifndef VKPONG_VULKAN_DEVICE_INCLUDED
#define VKPONG_VULKAN_DEVICE_INCLUDED

#include <vulkan/vulkan_core.h>

#include <utility>

namespace vkpong
{
    class vulkan_context;
} // namespace vkpong

namespace vkpong
{
    class [[nodiscard]] vulkan_device final
    {
    public: // Construction
        vulkan_device(VkPhysicalDevice physical_device,
            VkDevice logical_device,
            uint32_t graphics_family,
            uint32_t present_family);

        vulkan_device(vulkan_device const&) = delete;

        vulkan_device(vulkan_device&& other) noexcept;

    public: // Destruction
        ~vulkan_device();

    public: // Interface
        [[nodiscard]] constexpr VkPhysicalDevice physical() const noexcept;

        [[nodiscard]] constexpr VkDevice logical() const noexcept;

        [[nodiscard]] constexpr uint32_t graphics_family() const noexcept;

        [[nodiscard]] constexpr uint32_t present_family() const noexcept;

        [[nodiscard]] constexpr VkSampleCountFlagBits
        max_msaa_samples() const noexcept;

    public: // Operators
        vulkan_device& operator=(vulkan_device const&) = delete;

        vulkan_device& operator=(vulkan_device&& other) noexcept;

    private: // Data
        VkPhysicalDevice physical_device_{};
        VkDevice logical_device_{};
        uint32_t graphics_family_{};
        uint32_t present_family_{};
        VkSampleCountFlagBits max_msaa_samples_{VK_SAMPLE_COUNT_1_BIT};
    };

    vulkan_device create_device(vulkan_context const& context);
} // namespace vkpong

inline vkpong::vulkan_device::vulkan_device(vulkan_device&& other) noexcept
    : physical_device_{other.physical_device_}
    , logical_device_{std::exchange(other.logical_device_, nullptr)}
    , graphics_family_{other.graphics_family_}
    , present_family_{other.present_family_}
    , max_msaa_samples_{other.max_msaa_samples_}
{
}

inline constexpr VkPhysicalDevice
vkpong::vulkan_device::physical() const noexcept
{
    return physical_device_;
}

inline constexpr VkDevice vkpong::vulkan_device::logical() const noexcept
{
    return logical_device_;
}

inline constexpr uint32_t
vkpong::vulkan_device::graphics_family() const noexcept
{
    return graphics_family_;
}

inline constexpr uint32_t vkpong::vulkan_device::present_family() const noexcept
{
    return present_family_;
}

inline constexpr VkSampleCountFlagBits
vkpong::vulkan_device::max_msaa_samples() const noexcept
{
    return max_msaa_samples_;
}

inline vkpong::vulkan_device& vkpong::vulkan_device::operator=(
    vulkan_device&& other) noexcept
{
    using std::swap;

    if (this != &other)
    {
        swap(physical_device_, other.physical_device_);
        swap(logical_device_, other.logical_device_);
        swap(graphics_family_, other.graphics_family_);
        swap(present_family_, other.present_family_);
        swap(max_msaa_samples_, other.max_msaa_samples_);
    }

    return *this;
}

#endif // !VKPONG_VULKAN_DEVICE_INCLUDED
