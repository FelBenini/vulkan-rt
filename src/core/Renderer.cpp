#include "core/Renderer.hpp"
#include "platform/Window.hpp"
#include "core/VertexBuffer.hpp"
#include <iostream>
#include <limits>
#include <vector>

Renderer::Renderer(Window&                   window,
                   const vk::raii::Instance& instance,
                   vk::raii::SurfaceKHR      surface)
    : m_window(window)
    , m_surface(std::move(surface))
    , m_device(instance, m_surface)
    , m_swapchain(m_device, m_surface, m_window)
    , m_commandPool(m_device)
    , m_sync(m_device, 2)
    , m_pipeline(m_device, m_swapchain)
    , m_framebuffer(m_device, m_swapchain, m_pipeline)
{
    std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
        {{0.75f, 0.75f}, {1.0f, 1.0f, 1.0f}}
    };
    m_vertexBuffer = VertexBuffer(m_device, vertices);
}

void Renderer::drawFrame() {
    // Wait for the previous frame to finish
    auto result = m_device.getLogical().waitForFences(
        {*m_sync.getInFlightFence(m_currentFrame)},
        true,
        std::numeric_limits<uint64_t>::max()
    );
    if (result != vk::Result::eSuccess) {
        return;
    }

    // Handle explicit resize flag from window events
    if (m_window.wasResized()) {
        m_window.clearResizeFlag();
        recreateSwapchain();
        return;
    }

    // Acquire next image
    auto imageIndex = m_swapchain.acquireNextImage(
        m_sync.getImageAvailableSemaphore(m_currentFrame)
    );

    if (!imageIndex) {
        recreateSwapchain();
        return;
    }

    // Reset fence for this frame
    m_device.getLogical().resetFences({*m_sync.getInFlightFence(m_currentFrame)});

    // Record command buffer
    auto& cmdBuffer = m_commandPool.getBuffer(m_currentFrame);
    cmdBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    vk::CommandBuffer underlying = *cmdBuffer;
    recordCommandBuffer(underlying, *imageIndex);

    // Submit to queue
    vk::CommandBuffer cmd = *m_commandPool.getBuffer(m_currentFrame);
    vk::Semaphore waitSemaphore = *m_sync.getImageAvailableSemaphore(m_currentFrame);
    vk::Semaphore signalSemaphore = *m_sync.getRenderFinishedSemaphore(m_currentFrame);
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    const vk::SubmitInfo submitInfo(
        1, &waitSemaphore,
        &waitStage,
        1, &cmd,
        1, &signalSemaphore
    );

    m_device.getGraphicsQueue().submit(submitInfo, *m_sync.getInFlightFence(m_currentFrame));

    // Present
    bool ok = m_swapchain.present(
        m_device.getPresentQueue(),
        *imageIndex,
        m_sync.getRenderFinishedSemaphore(m_currentFrame)
    );

    if (!ok) {
        recreateSwapchain();
    }

    m_currentFrame = (m_currentFrame + 1) % m_sync.maxFramesInFlight();
}

void Renderer::recreateSwapchain() {
    // Wait for all in-flight GPU work to finish before destroying/recreating
    m_device.getLogical().waitIdle();

    // Handle minimized window — spin until the window has a valid size
    auto [w, h] = m_window.getDrawableSize();
    while (w == 0 || h == 0) {
        m_window.pollEvents();
        std::tie(w, h) = m_window.getDrawableSize();
    }

    // Replace the swapchain in-place
    m_swapchain = Swapchain(m_device, m_surface, m_window);

    // Recreate pipeline with new swapchain extent
    m_pipeline = Pipeline(m_device, m_swapchain);

    // Recreate framebuffers
    m_framebuffer = Framebuffer(m_device, m_swapchain, m_pipeline);

    std::cout << "[Renderer] Swapchain recreated (" << w << "x" << h << ")\n";
}

void Renderer::recordCommandBuffer(vk::CommandBuffer& buffer, uint32_t imageIndex) {
    const vk::CommandBufferBeginInfo beginInfo;
    buffer.begin(beginInfo);

    // Black clear
    vk::ClearValue clearValue(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));

    const vk::RenderPassBeginInfo renderPassInfo(
        *m_pipeline.getRenderPass(),
        *m_framebuffer.get(imageIndex),
        vk::Rect2D({0, 0}, m_swapchain.getExtent()),
        1,
        &clearValue
    );

    buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline.getPipeline());
    
    vk::Buffer vertexBuffer = *m_vertexBuffer.getBuffer();
    vk::DeviceSize offset = 0;
    buffer.bindVertexBuffers(0, 1, &vertexBuffer, &offset);
    
    // Try with vertexCount parameter directly
    buffer.draw(m_vertexBuffer.getCount(), 1, 0, 0);
    buffer.endRenderPass();
    buffer.end();
}