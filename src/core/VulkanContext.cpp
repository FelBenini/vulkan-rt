#include "core/VulkanContext.hpp"
#include <iostream>

VulkanContext::VulkanContext(const std::vector<const char*>& requiredExtensions, bool enableValidation)
    : m_context() 
    , m_instance(m_context, createInstanceInfo(requiredExtensions, enableValidation))
{
    if (enableValidation) {
        // Use the constructor: vk::DebugUtilsMessengerCreateInfoEXT(flags, severity, type, callback, userData)
        vk::DebugUtilsMessengerCreateInfoEXT debugInfo(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            &debugCallback
        );
        m_debugMessenger = vk::raii::DebugUtilsMessengerEXT(m_instance, debugInfo);
    }
}

vk::InstanceCreateInfo VulkanContext::createInstanceInfo(const std::vector<const char*>& requiredExtensions, bool enableValidation) {
    // We use static/long-lived containers because Vulkan structs store pointers to them
    static std::vector<const char*> layers;
    layers.clear();
    static std::vector<const char*> enabledExtensions;
    enabledExtensions.clear();
    if (enableValidation) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
        enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    enabledExtensions.insert(enabledExtensions.end(), requiredExtensions.begin(), requiredExtensions.end());

    // Constructor: vk::ApplicationInfo(appName, appVer, engineName, engineVer, apiVer)
    static vk::ApplicationInfo appInfo(
        "Vulkan Cube", 1,
        "No Engine", 1,
        VK_API_VERSION_1_3
    );

    // Constructor: vk::InstanceCreateInfo(flags, pAppInfo, layerCount, pLayerNames, extCount, pExtNames)
    return vk::InstanceCreateInfo(
        {},
        &appInfo,
        static_cast<uint32_t>(layers.size()),
        layers.data(),
        static_cast<uint32_t>(enabledExtensions.size()),
        enabledExtensions.data()
    );
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
    VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/) 
{
    std::cerr << "[Validation Layer] " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}