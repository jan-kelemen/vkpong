#include <vulkan_device.hpp>

#include <vulkan_context.hpp>

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

    struct [[nodiscard]] queue_family_indices
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

    struct [[nodiscard]] swap_chain_support_details
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    swap_chain_support_details query_swap_chain_support(VkPhysicalDevice device,
        VkSurfaceKHR surface)
    {
        swap_chain_support_details details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device,
            surface,
            &details.capabilities);

        uint32_t format_count{};
        vkGetPhysicalDeviceSurfaceFormatsKHR(device,
            surface,
            &format_count,
            nullptr);
        if (format_count != 0)
        {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device,
                surface,
                &format_count,
                details.formats.data());
        }

        uint32_t present_count{};
        vkGetPhysicalDeviceSurfacePresentModesKHR(device,
            surface,
            &present_count,
            nullptr);
        if (present_count != 0)
        {
            details.present_modes.resize(present_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device,
                surface,
                &present_count,
                details.present_modes.data());
        }

        return details;
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
        VkSurfaceKHR surface)
    {
        auto const indices{find_queue_families(device, surface)};

        bool swap_chain_adequate = false;
        if (extensions_supported(device))
        {
            auto const swap_chain_support{
                query_swap_chain_support(device, surface)};

            swap_chain_adequate = !swap_chain_support.formats.empty() &&
                !swap_chain_support.present_modes.empty();
        }

        VkPhysicalDeviceFeatures supported_features;
        vkGetPhysicalDeviceFeatures(device, &supported_features);

        return indices.graphics_family && indices.present_family &&
            swap_chain_adequate && supported_features.samplerAnisotropy;
    }
} // namespace

vkpong::vulkan_device::~vulkan_device() { vkDestroyDevice(logical, nullptr); }

std::unique_ptr<vkpong::vulkan_device> vkpong::create_device(
    vulkan_context* context)
{
    uint32_t count{};
    vkEnumeratePhysicalDevices(context->instance, &count, nullptr);
    if (count == 0)
    {
        throw std::runtime_error{"failed to find GPUs with Vulkan support!"};
    }

    std::vector<VkPhysicalDevice> devices{count};
    vkEnumeratePhysicalDevices(context->instance, &count, devices.data());
    auto const device_it{std::ranges::find_if(devices,
        [context](auto const& device)
        { return is_device_suitable(device, context->surface); })};
    if (device_it == devices.cend())
    {
        throw std::runtime_error{"failed to find a suitable GPU!"};
    }

    auto rv{std::make_unique<vulkan_device>()};
    rv->physical = *device_it;

    queue_family_indices const device_indices{
        find_queue_families(rv->physical, context->surface)};
    rv->graphics_family = device_indices.graphics_family.value_or(0);
    rv->present_family = device_indices.present_family.value_or(0);

    float const priority{1.0f};
    std::set<uint32_t> const unique_families{rv->graphics_family,
        rv->present_family};
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
    create_info.queueCreateInfoCount =
        static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.enabledLayerCount = 0;
    create_info.enabledExtensionCount =
        static_cast<uint32_t>(device_extensions.size());
    create_info.ppEnabledExtensionNames = device_extensions.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.pNext = &device_13_features;
    if (vkCreateDevice(rv->physical, &create_info, nullptr, &rv->logical) !=
        VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create logical device!"};
    }

    return rv;
}
