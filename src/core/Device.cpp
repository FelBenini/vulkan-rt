#include "core/Device.hpp"
#include <iostream>
#include <set>
#include <stdexcept>
#include <algorithm>

// Constructor

Device::Device(const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface) {
    pickPhysicalDevice(instance, surface);
    createLogicalDevice();
}

// Physical Device Selection
void Device::pickPhysicalDevice(const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface)
{

    vk::raii::PhysicalDevices candidates(instance);

    if (candidates.empty())
    {
        throw std::runtime_error("No Vulkan-capable GPU found.");
    }

    int    bestScore  = -1;
    size_t bestIndex  = 0;
    // Pick the best graphics card available.
    for (size_t i = 0; i < candidates.size(); ++i) {
        int score = scorePhysicalDevice(candidates[i], surface);
        std::cout << "[Device] Found GPU: "
                  << candidates[i].getProperties().deviceName
                  << "  (score: " << score << ")\n";
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    if (bestScore < 0)
    {
        throw std::runtime_error("No suitable GPU found (missing required extensions or queue families).");
    }

    m_physicalDevice = std::move(candidates[bestIndex]);
    m_queueIndices   = findQueueFamilies(m_physicalDevice, surface);
    m_capabilities.rayTracing       = supportsRayTracing(m_physicalDevice);
    m_capabilities.dynamicRendering = true; // Required extension guarantees this

    std::cout << "[Device] Selected GPU: "
              << m_physicalDevice.getProperties().deviceName
              << (m_capabilities.rayTracing ? "  [Ray Tracing: YES]" : "  [Ray Tracing: NO]")
              << "\n";
}

int Device::scorePhysicalDevice(const vk::raii::PhysicalDevice& device,
                                 const vk::raii::SurfaceKHR& surface) const {
    // Disqualify if required extensions or queue families are missing
    if (!supportsRequiredExtensions(device)) return -1;

    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    if (!indices.isComplete()) return -1;

    // Check swapchain adequacy (needs at least one format and present mode)
    auto formats      = device.getSurfaceFormatsKHR(*surface);
    auto presentModes = device.getSurfacePresentModesKHR(*surface);
    if (formats.empty() || presentModes.empty()) return -1;

    int score = 0;

    // Strongly prefer discrete GPU
    auto props = device.getProperties();
    if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)   score += 10000;
    if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) score += 1000;
    if (props.deviceType == vk::PhysicalDeviceType::eVirtualGpu)    score += 500;

    // Bonus for ray tracing support
    if (supportsRayTracing(device)) score += 5000;

    // Bonus for dedicated transfer queue (better async performance)
    if (indices.transfer.has_value()) score += 200;

    // Bonus for larger max image dimension (a rough proxy for GPU capability)
    score += static_cast<int>(props.limits.maxImageDimension2D / 256);

    return score;
}

bool Device::supportsRequiredExtensions(const vk::raii::PhysicalDevice& device) const {
    auto available = device.enumerateDeviceExtensionProperties();
    for (const char* required : k_requiredExtensions) {
        bool found = std::any_of(available.begin(), available.end(),
            [required](const vk::ExtensionProperties& ext) {
                return std::string_view(ext.extensionName) == required;
            });
        if (!found) return false;
    }
    return true;
}

bool Device::supportsRayTracing(const vk::raii::PhysicalDevice& device) const {
    auto available = device.enumerateDeviceExtensionProperties();
    for (const char* rtExt : k_rayTracingExtensions) {
        bool found = std::any_of(available.begin(), available.end(),
            [rtExt](const vk::ExtensionProperties& ext) {
                return std::string_view(ext.extensionName) == rtExt;
            });
        if (!found) return false;
    }
    return true;
}

// Queue Family Resolution

QueueFamilyIndices Device::findQueueFamilies(const vk::raii::PhysicalDevice& device,
                                              const vk::raii::SurfaceKHR& surface) const {
    QueueFamilyIndices indices;
    auto families = device.getQueueFamilyProperties();

    for (uint32_t i = 0; i < static_cast<uint32_t>(families.size()); ++i) {
        const auto& family = families[i];

        // Graphics queue
        if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
            if (!indices.graphics.has_value()) indices.graphics = i;
        }

        // Present support
        if (device.getSurfaceSupportKHR(i, *surface)) {
            if (!indices.present.has_value()) indices.present = i;
        }

        // Prefer a dedicated compute queue (no graphics bit)
        if ((family.queueFlags & vk::QueueFlagBits::eCompute) &&
            !(family.queueFlags & vk::QueueFlagBits::eGraphics)) {
            indices.compute = i;
        }

        // Dedicated transfer queue (no graphics or compute)
        if ((family.queueFlags & vk::QueueFlagBits::eTransfer) &&
            !(family.queueFlags & vk::QueueFlagBits::eGraphics) &&
            !(family.queueFlags & vk::QueueFlagBits::eCompute)) {
            indices.transfer = i;
        }
    }

    // Fallback: use graphics queue for compute if no dedicated one was found
    if (!indices.compute.has_value()) indices.compute = indices.graphics;

    return indices;
}

// Logical Device Creation

void Device::createLogicalDevice() {
    // Collect unique queue family indices
    std::set<uint32_t> uniqueFamilies = {
        m_queueIndices.graphics.value(),
        m_queueIndices.present.value(),
        m_queueIndices.compute.value(),
    };
    if (m_queueIndices.transfer.has_value())
        uniqueFamilies.insert(m_queueIndices.transfer.value());

    float queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueFamilies.size());
    for (uint32_t family : uniqueFamilies) {
        queueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags{}, family, 1, &queuePriority);
    }

    // --- Build extension list ---
    std::vector<const char*> extensions(k_requiredExtensions.begin(), k_requiredExtensions.end());
    if (m_capabilities.rayTracing) {
        extensions.insert(extensions.end(),
                          k_rayTracingExtensions.begin(), k_rayTracingExtensions.end());
    }

    // --- Feature chain (pNext) ---
    // Vulkan 1.2 features
    vk::PhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = m_capabilities.rayTracing ? VK_TRUE : VK_FALSE;
    features12.descriptorIndexing  = VK_TRUE;

    // Vulkan 1.3 features (includes dynamic rendering)
    vk::PhysicalDeviceVulkan13Features features13{};
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE; // Strongly recommended alongside dynamic rendering
    features13.pNext = &features12; // Wait — pNext goes upward in the chain; see below

    // Ray tracing features (only populated if supported)
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{};
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures{};

    // Build pNext chain: deviceCreateInfo -> features13 -> features12 -> [rt features]
    // We use a raw void* chain — each struct's pNext points to the next
    features12.pNext = nullptr;
    features13.pNext = &features12;

    void* pNextChainHead = &features13;

    if (m_capabilities.rayTracing) {
        rtPipelineFeatures.rayTracingPipeline = VK_TRUE;
        accelFeatures.accelerationStructure   = VK_TRUE;

        // Prepend RT features to the chain
        accelFeatures.pNext    = pNextChainHead;
        rtPipelineFeatures.pNext = &accelFeatures;
        pNextChainHead           = &rtPipelineFeatures;
    }

    // Basic features (Vulkan 1.0)
    vk::PhysicalDeviceFeatures baseFeatures{};
    baseFeatures.samplerAnisotropy = VK_TRUE;
    baseFeatures.shaderInt64       = VK_TRUE; // Needed for ray tracing address arithmetic

    vk::DeviceCreateInfo createInfo{};
    createInfo.pNext                   = pNextChainHead;
    createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos       = queueCreateInfos.data();
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.pEnabledFeatures        = &baseFeatures;

    m_device = vk::raii::Device(m_physicalDevice, createInfo);

    // Retrieve queue handles
    m_graphicsQueue = m_device.getQueue(m_queueIndices.graphics.value(), 0);
    m_presentQueue  = m_device.getQueue(m_queueIndices.present.value(),  0);
    m_computeQueue  = m_device.getQueue(m_queueIndices.compute.value(),  0);
    if (m_queueIndices.transfer.has_value())
        m_transferQueue = m_device.getQueue(m_queueIndices.transfer.value(), 0);

    std::cout << "[Device] Logical device created.\n"
              << "  Graphics family : " << m_queueIndices.graphics.value() << "\n"
              << "  Present family  : " << m_queueIndices.present.value()  << "\n"
              << "  Compute family  : " << m_queueIndices.compute.value()  << "\n";
    if (m_queueIndices.transfer.has_value())
        std::cout << "  Transfer family : " << m_queueIndices.transfer.value() << "\n";
}