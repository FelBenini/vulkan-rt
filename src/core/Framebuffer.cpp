#include "core/Framebuffer.hpp"

Framebuffer::Framebuffer(const Device& device, const Swapchain& swapchain, const Pipeline& pipeline) {
    const auto& dev = device.getLogical();
    const auto& extent = swapchain.getExtent();
    const auto& imageViews = swapchain.getImageViews();
    const auto& renderPass = pipeline.getRenderPass();
    const auto& depthView = swapchain.getDepthImageView();

    std::array attachments = {*imageViews[0], *depthView};

    m_framebuffers.reserve(imageViews.size());
    for (size_t i = 0; i < imageViews.size(); ++i) {
        attachments[0] = *imageViews[i];
        vk::FramebufferCreateInfo fbInfo(
            {},
            *renderPass,
            static_cast<uint32_t>(attachments.size()),
            attachments.data(),
            extent.width,
            extent.height,
            1
        );
        m_framebuffers.emplace_back(dev, fbInfo);
    }
}
