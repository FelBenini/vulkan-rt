#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <optional>
#include <vector>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;

    bool isComplete() const {
        return graphics.has_value() && present.has_value() && compute.has_value();
    }
};

struct DeviceCapabilities {
    bool rayTracing      = false;
    bool meshShaders     = false;
    bool dynamicRendering = false; // Required
};

class Device {
public:
    Device(const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface);

    // Accessors
    const vk::raii::PhysicalDevice& getPhysical()  const { return m_physicalDevice; }
    const vk::raii::Device&         getLogical()   const { return m_device; }
    const vk::raii::Queue&          getGraphicsQueue() const { return m_graphicsQueue; }
    const vk::raii::Queue&          getPresentQueue()  const { return m_presentQueue; }
    const vk::raii::Queue&          getComputeQueue()  const { return m_computeQueue; }
    const QueueFamilyIndices&       getQueueIndices()  const { return m_queueIndices; }
    const DeviceCapabilities&       getCapabilities()  const { return m_capabilities; }

    bool hasRayTracingSupport()   const { return m_capabilities.rayTracing; }
    bool hasDedicatedTransfer()   const { return m_queueIndices.transfer.has_value(); }

private:
    // Required device extensions (always enabled)
    static constexpr std::array k_requiredExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    };

    // Optional ray-tracing extensions (enabled when available)
    static constexpr std::array k_rayTracingExtensions = {
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,  // Required by spirv_1_4
    };

    vk::raii::PhysicalDevice m_physicalDevice = nullptr;
    vk::raii::Device         m_device         = nullptr;
    vk::raii::Queue          m_graphicsQueue  = nullptr;
    vk::raii::Queue          m_presentQueue   = nullptr;
    vk::raii::Queue          m_computeQueue   = nullptr;
    vk::raii::Queue          m_transferQueue  = nullptr; // May be null if no dedicated family

    QueueFamilyIndices m_queueIndices;
    DeviceCapabilities m_capabilities;

    // --- Selection helpers ---
    void pickPhysicalDevice(const vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface);
    int  scorePhysicalDevice(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) const;

    bool supportsRequiredExtensions(const vk::raii::PhysicalDevice& device) const;
    bool supportsRayTracing(const vk::raii::PhysicalDevice& device) const;

    QueueFamilyIndices findQueueFamilies(const vk::raii::PhysicalDevice& device,
                                         const vk::raii::SurfaceKHR& surface) const;

    // --- Logical device creation ---
    void createLogicalDevice();
};