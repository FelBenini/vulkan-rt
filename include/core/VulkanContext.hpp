#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vector>

class VulkanContext {
public:
    VulkanContext(const std::vector<const char*>& requiredExtensions, bool enableValidation = true);

    const vk::raii::Instance& getInstance() const { return m_instance; }

private:
    vk::raii::Context m_context; // Must be first
    vk::raii::Instance m_instance;
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;

    vk::InstanceCreateInfo createInstanceInfo(const std::vector<const char*>& extensions, bool enableValidation);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};
