#include "core/Renderer.hpp"
#include "platform/Window.hpp"
#include "camera/Camera.hpp"
#include "core/VertexBuffer.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <iostream>
#include <limits>
#include <vector>

Renderer::Renderer(Window&                   window,
                   const vk::raii::Instance& instance,
                   vk::raii::SurfaceKHR      surface)
    : m_window(window)
    , m_surface(std::move(surface))
    , m_device(instance, m_surface)
    , m_swapchain(m_device, m_surface, m_window, m_device.getMsaaSamples())
    , m_commandPool(m_device, m_swapchain.imageCount())
    , m_sync(m_device, m_swapchain.imageCount())
    , m_pipeline(m_device, m_swapchain)
    , m_framebuffer(m_device, m_swapchain, m_pipeline)
{
    // 3D Cube with 36 vertices (6 faces * 2 triangles * 3 vertices)
    std::vector<Vertex> vertices = {
        // Front face (z = +0.5)
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        
        // Back face (z = -0.5)
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        
        // Top face (y = +0.5)
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
        
        // Bottom face (y = -0.5)
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}},
        
        // Right face (x = +0.5)
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        
        // Left face (x = -0.5)
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
    };
    m_vertexBuffer = VertexBuffer(m_device, vertices);
}

void Renderer::drawFrame(const Camera& camera) {
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
    recordCommandBuffer(underlying, *imageIndex, camera);

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
    m_swapchain = Swapchain(m_device, m_surface, m_window, m_device.getMsaaSamples());

    // Recreate command pool and sync with new image count
    m_commandPool = CommandPool(m_device, m_swapchain.imageCount());
    m_sync = Sync(m_device, m_swapchain.imageCount());
    m_currentFrame = 0;

    // Recreate pipeline with new swapchain extent
    m_pipeline = Pipeline(m_device, m_swapchain);

    // Recreate framebuffers
    m_framebuffer = Framebuffer(m_device, m_swapchain, m_pipeline);

    std::cout << "[Renderer] Swapchain recreated (" << w << "x" << h << ")\n";
}

void Renderer::recordCommandBuffer(vk::CommandBuffer& buffer, uint32_t imageIndex, const Camera& camera) {
    const vk::CommandBufferBeginInfo beginInfo;
    buffer.begin(beginInfo);

    std::array clearValues = {
        vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f})),
        vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0))
    };

    const vk::RenderPassBeginInfo renderPassInfo(
        *m_pipeline.getRenderPass(),
        *m_framebuffer.get(imageIndex),
        vk::Rect2D({0, 0}, m_swapchain.getExtent()),
        static_cast<uint32_t>(clearValues.size()),
        clearValues.data()
    );

    buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline.getPipeline());

    // Time-based rotation
    static auto lastTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    static float totalTime = 0.0f;
    float dt = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    totalTime += dt;

    // Model matrix with translation and rotation
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f));
    model = glm::rotate(model, totalTime, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, totalTime * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));

    // Push camera matrices
    glm::mat4 view = camera.getViewMatrix();
    float fov = camera.getFOV() * glm::pi<float>() / 180.0f;
    glm::mat4 proj = glm::perspective(fov, camera.getAspectRatio(), 0.1f, 100.0f);
    
    // Vulkan uses inverted Y in projection matrix
    proj[1][1] = -proj[1][1];

    struct PushConstants {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    } pc = {model, view, proj};

    buffer.pushConstants(
        *m_pipeline.getLayout(),
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(PushConstants),
        &pc
    );

    vk::Buffer vertexBuffer = *m_vertexBuffer.getBuffer();
    vk::DeviceSize offset = 0;
    buffer.bindVertexBuffers(0, 1, &vertexBuffer, &offset);
    
    buffer.draw(m_vertexBuffer.getCount(), 1, 0, 0);
    buffer.endRenderPass();
    buffer.end();
}