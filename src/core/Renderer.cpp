#include "core/Renderer.hpp"
#include "platform/Window.hpp"
#include <iostream>

Renderer::Renderer(Window&                   window,
                   const vk::raii::Instance& instance,
                   vk::raii::SurfaceKHR      surface)
    : m_window(window)
    , m_surface(std::move(surface))
    , m_device(instance, m_surface)
    , m_swapchain(m_device, m_surface, m_window)
{
}

void Renderer::drawFrame() {
    // Handle explicit resize flag from window events
    if (m_window.wasResized()) {
        m_window.clearResizeFlag();
        recreateSwapchain();
        return;
    }

    // TODO (next step): acquire image, record commands, submit, present
    // Pseudocode for what comes here:
    //
    // auto imageIndex = m_swapchain.acquireNextImage(imageAvailableSemaphore);
    // if (!imageIndex) { recreateSwapchain(); return; }
    //
    // recordCommandBuffer(*imageIndex);
    // submitCommandBuffer();
    //
    // bool ok = m_swapchain.present(m_device.getPresentQueue(), *imageIndex, renderFinishedSemaphore);
    // if (!ok) recreateSwapchain();
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

    std::cout << "[Renderer] Swapchain recreated (" << w << "x" << h << ")\n";
}