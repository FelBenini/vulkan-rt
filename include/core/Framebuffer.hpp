#pragma once
#include "core/Device.hpp"
#include "core/Swapchain.hpp"
#include "core/Pipeline.hpp"
#include <vulkan/vulkan_raii.hpp>

class Framebuffer {
public:
    Framebuffer(const Device& device, const Swapchain& swapchain, const Pipeline& pipeline);

    Framebuffer(const Framebuffer&)            = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Framebuffer&&) noexcept = default;
    Framebuffer& operator=(Framebuffer&&) noexcept = default;

    const vk::raii::Framebuffer& get(uint32_t index) const { return m_framebuffers[index]; }

private:
    std::vector<vk::raii::Framebuffer> m_framebuffers;
};
