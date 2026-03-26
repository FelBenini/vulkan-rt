#pragma once
#include "core/Device.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vector>

class Window;

struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR        capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR>   presentModes;
};

class Swapchain {
public:
    Swapchain(const Device&              device,
              const vk::raii::SurfaceKHR& surface,
              const Window&              window);

    // Non-copyable
    Swapchain(const Swapchain&)            = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    // Movable
    Swapchain(Swapchain&&) noexcept = default;
    Swapchain& operator=(Swapchain&&) noexcept = default;
    
    // Accessors
    const vk::raii::SwapchainKHR&              get()           const { return m_swapchain; }
    const vk::raii::RenderPass&                getRenderPass() const { return m_renderPass; }
    const std::vector<vk::raii::ImageView>&    getImageViews() const { return m_imageViews; }
    const vk::raii::ImageView&                 getDepthImageView() const { return m_depthImageView; }
    vk::Format                                 getFormat()     const { return m_format; }
    vk::Extent2D                               getExtent()     const { return m_extent; }
    uint32_t                                   imageCount()    const { return static_cast<uint32_t>(m_imageViews.size()); }

    // Returns image index, or nullopt if swapchain is out of date (resize needed)
    std::optional<uint32_t> acquireNextImage(const vk::raii::Semaphore& imageAvailableSemaphore);

    // Returns false if swapchain is out of date (resize needed)
    bool present(const vk::raii::Queue&     presentQueue,
                 uint32_t                   imageIndex,
                 const vk::raii::Semaphore& renderFinishedSemaphore);

    static SwapchainSupportDetails querySupport(const vk::raii::PhysicalDevice& physicalDevice,
                                                const vk::raii::SurfaceKHR&     surface);

private:
    vk::raii::SwapchainKHR           m_swapchain = nullptr;
    vk::raii::RenderPass             m_renderPass = nullptr;
    vk::raii::Image                  m_depthImage = nullptr;
    vk::raii::DeviceMemory           m_depthMemory = nullptr;
    vk::raii::ImageView              m_depthImageView = nullptr;
    std::vector<vk::Image>           m_images;
    std::vector<vk::raii::ImageView> m_imageViews;
    vk::Format                       m_format;
    vk::Extent2D                     m_extent;

    static vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available);
    static vk::PresentModeKHR   choosePresentMode  (const std::vector<vk::PresentModeKHR>&   available);
    static vk::Extent2D         chooseExtent        (const vk::SurfaceCapabilitiesKHR&        capabilities,
                                                     const Window&                            window);
};