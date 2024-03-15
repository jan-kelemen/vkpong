#include <vulkan_context.hpp>
#include <vulkan_utility.hpp>

#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <string_view>
#include <vector>

namespace
{
    constexpr std::array<char const*, 1> const validation_layers{
        "VK_LAYER_KHRONOS_validation"};

    [[nodiscard]] bool check_validation_layer_support()
    {
        uint32_t count{};
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        std::vector<VkLayerProperties> available_layers{count};
        vkEnumerateInstanceLayerProperties(&count, available_layers.data());

        for (std::string_view layer_name : validation_layers)
        {
            if (!std::ranges::any_of(available_layers,
                    [layer_name](VkLayerProperties const& layer) noexcept
                    { return layer_name == layer.layerName; }))
            {
                return false;
            }
        }

        return true;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT const severity,
        [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT const type,
        VkDebugUtilsMessengerCallbackDataEXT const* const callback_data,
        [[maybe_unused]] void* const user_data)
    {
        switch (severity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            spdlog::debug(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            spdlog::info(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            spdlog::warn(callback_data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            spdlog::error(callback_data->pMessage);
            break;
        default:
            spdlog::error("Unrecognized severity {}. {}",
                static_cast<std::underlying_type_t<
                    VkDebugUtilsMessageSeverityFlagBitsEXT>>(severity),
                callback_data->pMessage);
            break;
        }

        return VK_FALSE;
    }

    void populate_debug_messenger_create_info(
        VkDebugUtilsMessengerCreateInfoEXT& info)
    {
        info = {};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = debug_callback;
    }

    VkResult create_debug_utils_messenger_ext(VkInstance const instance,
        VkDebugUtilsMessengerCreateInfoEXT const* const create_info,
        VkAllocationCallbacks const* const allocator,
        VkDebugUtilsMessengerEXT* const debug_messenger)
    {
        // NOLINTNEXTLINE
        auto const func{reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"))};

        if (func != nullptr)
        {
            return func(instance, create_info, allocator, debug_messenger);
        }

        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void destroy_debug_utils_messenger_ext(VkInstance const instance,
        VkDebugUtilsMessengerEXT const debug_messenger,
        VkAllocationCallbacks const* allocator)
    {
        // NOLINTNEXTLINE
        auto const func{reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance,
                "vkDestroyDebugUtilsMessengerEXT"))};

        if (func != nullptr)
        {
            func(instance, debug_messenger, allocator);
        }
    }

    VkDebugUtilsMessengerEXT create_debug_messenger(VkInstance const instance)
    {
        VkDebugUtilsMessengerCreateInfoEXT create_info{};
        populate_debug_messenger_create_info(create_info);

        VkDebugUtilsMessengerEXT rv{};
        if (create_debug_utils_messenger_ext(instance,
                &create_info,
                nullptr,
                &rv) != VK_SUCCESS)
        {
            throw std::runtime_error{"failed to create debug messenger!"};
        }

        return rv;
    }
} // namespace

vkpong::vulkan_context::~vulkan_context()
{
    vkDestroySurfaceKHR(instance, surface, nullptr);
    if (debug_messenger)
    {
        destroy_debug_utils_messenger_ext(instance, *debug_messenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
}

std::unique_ptr<vkpong::vulkan_context> vkpong::create_context(
    GLFWwindow* const window,
    bool const setup_validation_layers)
{
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vkpong";
    app_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    uint32_t glfw_extension_count{};
    char const** const glfw_extensions{
        glfwGetRequiredInstanceExtensions(&glfw_extension_count)};

    std::vector<char const*> required_extensions{glfw_extensions,
        glfw_extensions + glfw_extension_count};

    bool has_debug_utils_extension{setup_validation_layers};
    if (setup_validation_layers)
    {
        if (check_validation_layer_support())
        {
            create_info.enabledLayerCount =
                count_cast(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();

            required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
            populate_debug_messenger_create_info(debug_create_info);
            create_info.pNext = &debug_create_info;
        }
        else
        {
            spdlog::warn("Validation layers requested but not available!");
            has_debug_utils_extension = false;
        }
    }

    create_info.enabledExtensionCount = count_cast(required_extensions.size());
    create_info.ppEnabledExtensionNames = required_extensions.data();

    auto rv{std::make_unique<vulkan_context>()};
    if (vkCreateInstance(&create_info, nullptr, &rv->instance) != VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create instance"};
    }

    if (has_debug_utils_extension)
    {
        rv->debug_messenger = create_debug_messenger(rv->instance);
    }

    if (glfwCreateWindowSurface(rv->instance, window, nullptr, &rv->surface) !=
        VK_SUCCESS)
    {
        throw std::runtime_error{"failed to create window surface"};
    }

    return rv;
}
