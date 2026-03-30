#include "core/Swapchain.hpp"
#include "platform/Window.hpp"
#include <iostream>
#include <algorithm>
#include <limits>

// Constructor

Swapchain::Swapchain(const Device&               device,
                     const vk::raii::SurfaceKHR& surface,
                     const Window&               window,
                     vk::SampleCountFlagBits     msaaSamples)
    : m_msaaSamples(msaaSamples)
{
    auto support = querySupport(device.getPhysical(), surface);

    vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(support.formats);
    vk::PresentModeKHR   presentMode   = choosePresentMode(support.presentModes);
    vk::Extent2D         extent        = chooseExtent(support.capabilities, window);

    m_format = surfaceFormat.format;
    m_extent = extent;

    // Request one more image than the minimum to avoid stalling on driver
    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0)
        imageCount = std::min(imageCount, support.capabilities.maxImageCount);

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface          = *surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;

    // If graphics and present are different families, images must be shared
    const auto& indices = device.getQueueIndices();
    std::array<uint32_t, 2> queueFamilies = {
        indices.graphics.value(),
        indices.present.value()
    };

    if (indices.graphics.value() != indices.present.value()) {
        createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilies.data();
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform   = support.capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;
    createInfo.oldSwapchain   = nullptr; // Renderer handles recreation externally

    m_swapchain = vk::raii::SwapchainKHR(device.getLogical(), createInfo);

    // Create multisample color image (resolve target)
    vk::ImageCreateInfo colorImageInfo(
        {},
        vk::ImageType::e2D,
        m_format,
        vk::Extent3D(extent.width, extent.height, 1),
        1, 1,
        m_msaaSamples,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
        vk::SharingMode::eExclusive,
        {},
        vk::ImageLayout::eUndefined
    );
    m_colorImage = vk::raii::Image(device.getLogical(), colorImageInfo);

    auto colorMemReqs = m_colorImage.getMemoryRequirements();
    auto memProps = device.getPhysical().getMemoryProperties();
    uint32_t memoryTypeIndex = 0;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if (colorMemReqs.memoryTypeBits & (1 << i)) {
            if (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
                memoryTypeIndex = i;
                break;
            }
        }
    }
    vk::MemoryAllocateInfo colorAllocInfo(colorMemReqs.size, memoryTypeIndex);
    m_colorMemory = vk::raii::DeviceMemory(device.getLogical(), colorAllocInfo);
    m_colorImage.bindMemory(*m_colorMemory, 0);

    vk::ImageViewCreateInfo colorViewInfo(
        {},
        *m_colorImage,
        vk::ImageViewType::e2D,
        m_format,
        {},
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );
    m_colorImageView = vk::raii::ImageView(device.getLogical(), colorViewInfo);

    // Create multisample depth image
    vk::ImageCreateInfo depthImageInfo(
        {},
        vk::ImageType::e2D,
        vk::Format::eD32Sfloat,
        vk::Extent3D(extent.width, extent.height, 1),
        1, 1,
        m_msaaSamples,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::SharingMode::eExclusive,
        {},
        vk::ImageLayout::eUndefined
    );
    m_depthImage = vk::raii::Image(device.getLogical(), depthImageInfo);

    auto depthMemReqs = m_depthImage.getMemoryRequirements();
    vk::MemoryAllocateInfo depthAllocInfo(depthMemReqs.size, memoryTypeIndex);
    m_depthMemory = vk::raii::DeviceMemory(device.getLogical(), depthAllocInfo);
    m_depthImage.bindMemory(*m_depthMemory, 0);

    vk::ImageSubresourceRange depthSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1);
    vk::ImageViewCreateInfo depthViewInfo(
        {},
        *m_depthImage,
        vk::ImageViewType::e2D,
        vk::Format::eD32Sfloat,
        {},
        depthSubresourceRange
    );
    m_depthImageView = vk::raii::ImageView(device.getLogical(), depthViewInfo);

    // Create render pass with MSAA: multisampled color/depth + single-sampled resolve
    vk::AttachmentDescription colorAttachment(
        {},
        m_format,
        m_msaaSamples,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    );

    vk::AttachmentDescription depthAttachment(
        {},
        vk::Format::eD32Sfloat,
        m_msaaSamples,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    );

    vk::AttachmentDescription resolveAttachment(
        {},
        m_format,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR
    );

    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);
    vk::AttachmentReference depthAttachmentRef(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    vk::AttachmentReference resolveAttachmentRef(2, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass(
        {},
        vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        1, &colorAttachmentRef,
        &resolveAttachmentRef,
        &depthAttachmentRef,
        0, nullptr
    );

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
        {},
        vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead,
        {}
    );

    std::array attachments = {colorAttachment, depthAttachment, resolveAttachment};
    vk::RenderPassCreateInfo renderPassInfo(
        {},
        static_cast<uint32_t>(attachments.size()),
        attachments.data(),
        1, &subpass,
        1, &dependency
    );

    m_renderPass = vk::raii::RenderPass(device.getLogical(), renderPassInfo);

    // Retrieve image handles (lifetime managed by swapchain, not us)
    m_images = m_swapchain.getImages();

    // Create image views
    m_imageViews.reserve(m_images.size());
    for (const auto& image : m_images) {
        vk::ImageViewCreateInfo viewInfo{};
        viewInfo.image                           = image;
        viewInfo.viewType                        = vk::ImageViewType::e2D;
        viewInfo.format                          = m_format;
        viewInfo.components.r                    = vk::ComponentSwizzle::eIdentity;
        viewInfo.components.g                    = vk::ComponentSwizzle::eIdentity;
        viewInfo.components.b                    = vk::ComponentSwizzle::eIdentity;
        viewInfo.components.a                    = vk::ComponentSwizzle::eIdentity;
        viewInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        m_imageViews.emplace_back(device.getLogical(), viewInfo);
    }

    std::cout << "[Swapchain] Created: " << m_images.size() << " images, "
              << "format=" << vk::to_string(m_format) << ", "
              << "extent=" << m_extent.width << "x" << m_extent.height << ", "
              << "presentMode=" << vk::to_string(presentMode) << "\n";
}

// Acquire & Present

std::optional<uint32_t> Swapchain::acquireNextImage(const vk::raii::Semaphore& imageAvailableSemaphore) {
    constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();

    auto [result, imageIndex] = m_swapchain.acquireNextImage(timeout, *imageAvailableSemaphore);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        return std::nullopt; // Signal Renderer to recreate
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("Failed to acquire swapchain image: " + vk::to_string(result));
    }
    return imageIndex;
}

bool Swapchain::present(const vk::raii::Queue&     presentQueue,
                        uint32_t                   imageIndex,
                        const vk::raii::Semaphore& renderFinishedSemaphore) {
    vk::PresentInfoKHR presentInfo{};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &*renderFinishedSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &*m_swapchain;
    presentInfo.pImageIndices      = &imageIndex;

    vk::Result result = presentQueue.presentKHR(presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
        return false; // Signal Renderer to recreate
    }
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to present swapchain image: " + vk::to_string(result));
    }
    return true;
}

// Static helpers

SwapchainSupportDetails Swapchain::querySupport(const vk::raii::PhysicalDevice& physicalDevice,
                                                const vk::raii::SurfaceKHR&     surface) {
    return {
        .capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface),
        .formats      = physicalDevice.getSurfaceFormatsKHR(*surface),
        .presentModes = physicalDevice.getSurfacePresentModesKHR(*surface),
    };
}

vk::SurfaceFormatKHR Swapchain::chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available) {
    for (const auto& format : available) {
        if (format.format     == vk::Format::eB8G8R8A8Srgb &&
            format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }
    return available.front(); // Fallback
}

vk::PresentModeKHR Swapchain::choosePresentMode(const std::vector<vk::PresentModeKHR>& available) {
    for (const auto& mode : available) {
        if (mode == vk::PresentModeKHR::eMailbox) return mode; // Triple buffering
    }
    return vk::PresentModeKHR::eFifo; // VSync — guaranteed to exist
}

vk::Extent2D Swapchain::chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities,
                                      const Window&                     window) {
    // If currentExtent is not the sentinel value, the surface dictates the size
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    // Otherwise query the window's drawable size (handles HiDPI correctly)
    auto [w, h] = window.getDrawableSize();
    return {
        std::clamp(static_cast<uint32_t>(w),
                   capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width),
        std::clamp(static_cast<uint32_t>(h),
                   capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height),
    };
}