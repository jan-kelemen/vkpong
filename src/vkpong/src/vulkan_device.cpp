#include <vulkan_device.hpp>

#include <vulkan_context.hpp>
#include <vulkan_swap_chain.hpp>
#include <vulkan_utility.hpp>

#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace
{
    constexpr std::array const device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};

    constexpr VkPhysicalDeviceFeatures device_features{
        .sampleRateShading = VK_TRUE,
        .samplerAnisotropy = VK_TRUE};

    constexpr VkPhysicalDeviceVulkan13Features device_13_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = VK_TRUE};

    struct [[nodiscard]] queue_family_indices final
    {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
    };

    queue_family_indices find_queue_families(VkPhysicalDevice device,
        VkSurfaceKHR surface)
    {
        queue_family_indices indices;

        uint32_t count{};
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families{count};
        vkGetPhysicalDeviceQueueFamilyProperties(device,
            &count,
            queue_families.data());

        uint32_t i{};
        for (auto const& queue_family : queue_families)
        {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family = i;
            }

            VkBool32 present_support{VK_FALSE};
            vkGetPhysicalDeviceSurfaceSupportKHR(device,
                i,
                surface,
                &present_support);

            if (present_support)
            {
                indices.present_family = i;
            }

            if (indices.graphics_family && indices.present_family)
            {
                break;
            }

            i++;
        }

        return indices;
    }

    [[nodiscard]] bool extensions_supported(VkPhysicalDevice device)
    {
        uint32_t count{};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

        std::vector<VkExtensionProperties> available_extensions{count};
        vkEnumerateDeviceExtensionProperties(device,
            nullptr,
            &count,
            available_extensions.data());

        std::set<std::string_view> required_extensions(
            device_extensions.cbegin(),
            device_extensions.cend());
        for (auto const& extension : available_extensions)
        {
            required_extensions.erase(extension.extensionName);
        }

        return required_extensions.empty();
    }

    [[nodiscard]] bool is_device_suitable(VkPhysicalDevice device,
        VkSurfaceKHR surface,
        queue_family_indices& indices)
    {
        if (!extensions_supported(device))
        {
            return false;
        }

        auto families{find_queue_families(device, surface)};
        bool const has_queue_families{
            families.graphics_family && families.present_family};
        if (!has_queue_families)
        {
            return false;
        }

        auto swap_chain{vkpong::query_swap_chain_support(device, surface)};
        bool const swap_chain_adequate = {!swap_chain.surface_formats.empty() &&
            !swap_chain.present_modes.empty()};
        if (!swap_chain_adequate)
        {
            return false;
        }

        VkPhysicalDeviceFeatures supported_features{};
        vkGetPhysicalDeviceFeatures(device, &supported_features);
        bool const features_adequate{
            supported_features.samplerAnisotropy == VK_TRUE};
        if (!features_adequate)
        {
            return false;
        }

        indices = std::move(families);

        return true;
    }

    [[nodiscard]] VkSampleCountFlagBits max_usable_sample_count(
        VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        VkSampleCountFlags const counts =
            properties.limits.framebufferColorSampleCounts &
            properties.limits.framebufferDepthSampleCounts;
        for (auto count : {VK_SAMPLE_COUNT_64_BIT,
                 VK_SAMPLE_COUNT_32_BIT,
                 VK_SAMPLE_COUNT_16_BIT,
                 VK_SAMPLE_COUNT_8_BIT,
                 VK_SAMPLE_COUNT_4_BIT,
                 VK_SAMPLE_COUNT_2_BIT})
        {
            if (counts & count)
            {
                return count;
            }
        }
        return VK_SAMPLE_COUNT_1_BIT;
    }
} // namespace

vkpong::vulkan_device::vulkan_device(VkPhysicalDevice physical_device,
    VkDevice logical_device,
    uint32_t graphics_family,
    uint32_t present_family)
    : physical_device_{physical_device}
    , logical_device_{logical_device}
    , graphics_family_{graphics_family}
    , present_family_{present_family}
    , max_msaa_samples_{max_usable_sample_count(physical_device)}
{
}

vkpong::vulkan_device::vulkan_device(vulkan_device&& other) noexcept
    : physical_device_{other.physical_device_}
    , logical_device_{std::exchange(other.logical_device_, nullptr)}
    , graphics_family_{other.graphics_family_}
    , present_family_{other.present_family_}
    , max_msaa_samples_{other.max_msaa_samples_}
{
}

vkpong::vulkan_device::~vulkan_device()
{
    vkDestroyDevice(logical_device_, nullptr);
}

vkpong::vulkan_device& vkpong::vulkan_device::operator=(
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

vkpong::vulkan_device vkpong::create_device(vulkan_context const& context)
{
    uint32_t count{};
    vkEnumeratePhysicalDevices(context.instance(), &count, nullptr);
    if (count == 0)
    {
        throw std::runtime_error{"failed to find GPUs with Vulkan support!"};
    }

    std::vector<VkPhysicalDevice> devices{count};
    vkEnumeratePhysicalDevices(context.instance(), &count, devices.data());

    queue_family_indices device_indices;
    auto const device_it{std::ranges::find_if(devices,
        [&context, &device_indices](auto const& device) mutable {
            return is_device_suitable(device,
                context.surface(),
                device_indices);
        })};
    if (device_it == devices.cend())
    {
        throw std::runtime_error{"failed to find a suitable GPU!"};
    }

    auto const graphics_family{device_indices.graphics_family.value_or(0)};
    auto const present_family{device_indices.present_family.value_or(0)};

    float const priority{1.0f};
    std::set<uint32_t> const unique_families{graphics_family, present_family};
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    for (uint32_t const family : unique_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;

        queue_create_infos.push_back(queue_create_info);
    }

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = count_cast(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.enabledLayerCount = 0;
    create_info.enabledExtensionCount = count_cast(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.pNext = &device_13_features;

    VkDevice logical_device{};
    if (vkCreateDevice(*device_it, &create_info, nullptr, &logical_device) !=
        VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create logical device!"};
    }

    return {*device_it, logical_device, graphics_family, present_family};
}
