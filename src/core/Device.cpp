#include "core/Device.hpp"
#include <iostream>
#include <set>
#include <stdexcept>
#include <algorithm>

Device::Device(const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface) {
    pickPhysicalDevice(instance, surface);
    createLogicalDevice();
}

// ---------------- PHYSICAL DEVICE ----------------

void Device::pickPhysicalDevice(const vk::raii::Instance& instance,
                                const vk::raii::SurfaceKHR& surface)
{
    vk::raii::PhysicalDevices candidates(instance);

    if (candidates.empty())
        throw std::runtime_error("No Vulkan-capable GPU found.");

    int bestScore = -1;
    size_t bestIndex = 0;

    for (size_t i = 0; i < candidates.size(); ++i) {
        int score = scorePhysicalDevice(candidates[i], surface);

        std::cout << "[Device] Found GPU: "
                  << candidates[i].getProperties().deviceName
                  << " (score: " << score << ")\n";

        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    if (bestScore < 0)
        throw std::runtime_error("No suitable GPU found.");

    m_physicalDevice = std::move(candidates[bestIndex]);
    m_queueIndices   = findQueueFamilies(m_physicalDevice, surface);
    m_capabilities.rayTracing = supportsRayTracing(m_physicalDevice);
    m_capabilities.dynamicRendering = true;

    // Query RT properties
    if (m_capabilities.rayTracing) {
        auto propsChain = m_physicalDevice.getProperties2<
            vk::PhysicalDeviceProperties2,
            vk::PhysicalDeviceRayTracingPipelinePropertiesKHR,
            vk::PhysicalDeviceAccelerationStructurePropertiesKHR
        >();

        m_rtPipelineProps = propsChain.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
        m_asProps         = propsChain.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
    }
    std::cout << "[Device] Selected GPU: "
              << m_physicalDevice.getProperties().deviceName
              << (m_capabilities.rayTracing ? " [RT: YES]" : " [RT: NO]")
              << "\n";
}

int Device::scorePhysicalDevice(const vk::raii::PhysicalDevice& device,
                                const vk::raii::SurfaceKHR& surface) const
{
    if (!supportsRequiredExtensions(device))
        return -1;

    QueueFamilyIndices indices = findQueueFamilies(device, surface);
    if (!indices.isComplete())
        return -1;

    auto formats = device.getSurfaceFormatsKHR(*surface);
    auto modes   = device.getSurfacePresentModesKHR(*surface);

    if (formats.empty() || modes.empty())
        return -1;

    int score = 0;
    auto props = device.getProperties();

    if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 10000;
    if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) score += 1000;

    if (supportsRayTracing(device)) score += 5000;
    if (indices.transfer.has_value()) score += 200;

    score += props.limits.maxImageDimension2D / 256;

    return score;
}

// ---------------- EXTENSIONS ----------------

bool Device::supportsRequiredExtensions(const vk::raii::PhysicalDevice& device) const {
    auto available = device.enumerateDeviceExtensionProperties();

    for (const char* required : k_requiredExtensions) {
        if (std::none_of(available.begin(), available.end(),
            [&](const vk::ExtensionProperties& ext) {
                return std::string_view(ext.extensionName) == required;
            }))
            return false;
    }
    return true;
}

bool Device::supportsRayTracing(const vk::raii::PhysicalDevice& device) const {
    auto available = device.enumerateDeviceExtensionProperties();

    for (const char* ext : k_rayTracingExtensions) {
        if (std::none_of(available.begin(), available.end(),
            [&](const vk::ExtensionProperties& e) {
                return std::string_view(e.extensionName) == ext;
            }))
            return false;
    }
    return true;
}

// ---------------- QUEUES ----------------

QueueFamilyIndices Device::findQueueFamilies(const vk::raii::PhysicalDevice& device,
                                             const vk::raii::SurfaceKHR& surface) const
{
    QueueFamilyIndices indices;
    auto families = device.getQueueFamilyProperties();

    for (uint32_t i = 0; i < families.size(); ++i) {
        const auto& f = families[i];

        if (f.queueFlags & vk::QueueFlagBits::eGraphics)
            if (!indices.graphics) indices.graphics = i;

        if (device.getSurfaceSupportKHR(i, *surface))
            if (!indices.present) indices.present = i;

        if ((f.queueFlags & vk::QueueFlagBits::eCompute) &&
            !(f.queueFlags & vk::QueueFlagBits::eGraphics))
            indices.compute = i;

        if ((f.queueFlags & vk::QueueFlagBits::eTransfer) &&
            !(f.queueFlags & vk::QueueFlagBits::eGraphics) &&
            !(f.queueFlags & vk::QueueFlagBits::eCompute))
            indices.transfer = i;
    }

    if (!indices.compute)
        indices.compute = indices.graphics;

    return indices;
}

// ---------------- LOGICAL DEVICE ----------------

void Device::createLogicalDevice() {
    std::set<uint32_t> families = {
        m_queueIndices.graphics.value(),
        m_queueIndices.present.value(),
        m_queueIndices.compute.value()
    };

    if (m_queueIndices.transfer)
        families.insert(m_queueIndices.transfer.value());

    float priority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueInfos;

    for (uint32_t f : families)
        queueInfos.emplace_back(vk::DeviceQueueCreateFlags{}, f, 1, &priority);

    std::vector<const char*> extensions(
        k_requiredExtensions.begin(), k_requiredExtensions.end());

    if (m_capabilities.rayTracing)
        extensions.insert(extensions.end(),
            k_rayTracingExtensions.begin(), k_rayTracingExtensions.end());

    // --- Features ---
    vk::PhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = m_capabilities.rayTracing;
    features12.descriptorIndexing = VK_TRUE;
    features12.runtimeDescriptorArray = VK_TRUE;
    features12.descriptorBindingPartiallyBound = VK_TRUE;
    features12.descriptorBindingVariableDescriptorCount = VK_TRUE;

    vk::PhysicalDeviceVulkan13Features features13{};
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accel{};
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rt{};

    if (m_capabilities.rayTracing) {
        accel.accelerationStructure = VK_TRUE;
        rt.rayTracingPipeline = VK_TRUE;

        accel.pNext = &rt;
        features12.pNext = &accel;
    }

    features13.pNext = &features12;

    vk::PhysicalDeviceFeatures base{};
    base.samplerAnisotropy = VK_TRUE;
    base.shaderInt64 = VK_TRUE;

    vk::DeviceCreateInfo createInfo{};
    createInfo.pNext = &features13;
    createInfo.queueCreateInfoCount = queueInfos.size();
    createInfo.pQueueCreateInfos = queueInfos.data();
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.pEnabledFeatures = &base;

    m_device = vk::raii::Device(m_physicalDevice, createInfo);

    m_graphicsQueue = m_device.getQueue(m_queueIndices.graphics.value(), 0);
    m_presentQueue  = m_device.getQueue(m_queueIndices.present.value(), 0);
    m_computeQueue  = m_device.getQueue(m_queueIndices.compute.value(), 0);

    if (m_queueIndices.transfer)
        m_transferQueue = m_device.getQueue(m_queueIndices.transfer.value(), 0);

    std::cout << "[Device] Logical device created\n";

    if (m_capabilities.rayTracing) {
        std::cout << "[RT] Max recursion: "
                  << m_rtPipelineProps.maxRayRecursionDepth << "\n";
        std::cout << "[RT] Handle size: "
                  << m_rtPipelineProps.shaderGroupHandleSize << "\n";
    }
}