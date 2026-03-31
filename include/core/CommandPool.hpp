#pragma once
#include "core/Device.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vector>

class CommandPool {
public:
    CommandPool(const Device& device, uint32_t bufferCount);

    CommandPool(const CommandPool&)            = delete;
    CommandPool& operator=(const CommandPool&) = delete;

    CommandPool(CommandPool&&) noexcept = default;
    CommandPool& operator=(CommandPool&&) noexcept = default;

    const vk::raii::CommandPool& getPool() const { return m_pool; }
    const vk::raii::CommandBuffer& getBuffer(uint32_t index) const { return m_buffers[index]; }
    uint32_t bufferCount() const { return static_cast<uint32_t>(m_buffers.size()); }

private:
    vk::raii::CommandPool             m_pool;
    std::vector<vk::raii::CommandBuffer> m_buffers;

    static vk::CommandPoolCreateInfo createInfo(uint32_t queueFamilyIndex);
};
