#pragma once
#include "core/Device.hpp"
#include "core/Swapchain.hpp"
#include "core/CommandPool.hpp"
#include "core/Sync.hpp"
#include "core/Pipeline.hpp"
#include "core/Framebuffer.hpp"
#include "core/VertexBuffer.hpp"
#include <vulkan/vulkan_raii.hpp>

class Window;

class Renderer {
public:
    Renderer(Window&                   window,
             const vk::raii::Instance& instance,
             vk::raii::SurfaceKHR      surface);

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    const Device&    getDevice()    const { return m_device; }
    const Swapchain& getSwapchain() const { return m_swapchain; }

    void drawFrame();

private:
    void recreateSwapchain();
    void recordCommandBuffer(vk::CommandBuffer& buffer, uint32_t imageIndex);

    // Declaration order = destruction order (reversed by RAII)
    // Surface must outlive Device; Swapchain must be destroyed before Device
    Window&              m_window;
    vk::raii::SurfaceKHR m_surface;   // (1) destroyed last among Vulkan objects
    Device               m_device;   // (2)
    Swapchain            m_swapchain; // (3) destroyed before device
    CommandPool          m_commandPool; // (4) destroyed before swapchain
    Sync                 m_sync;      // (5) destroyed before command pool
    Pipeline             m_pipeline;   // (6) destroyed before sync
    Framebuffer          m_framebuffer; // (7) destroyed before pipeline
    VertexBuffer         m_vertexBuffer; // (8) destroyed before framebuffer

    uint32_t m_currentFrame = 0;
};