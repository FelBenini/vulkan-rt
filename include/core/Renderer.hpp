#pragma once
#include "core/Device.hpp"
#include "core/Swapchain.hpp"
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
    // Declaration order = destruction order (reversed by RAII)
    // Surface must outlive Device; Swapchain must be destroyed before Device
    Window&              m_window;
    vk::raii::SurfaceKHR m_surface;   // (1) destroyed last among Vulkan objects
    Device               m_device;   // (2)
    Swapchain            m_swapchain; // (3) destroyed before device

    void recreateSwapchain();
};