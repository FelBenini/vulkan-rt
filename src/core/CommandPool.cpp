#include "core/CommandPool.hpp"
#include "core/Device.hpp"

CommandPool::CommandPool(const Device& device, uint32_t bufferCount)
    : m_pool(device.getLogical(), createInfo(device.getQueueIndices().graphics.value()))
    , m_buffers()
{
    const vk::CommandBufferAllocateInfo allocInfo(
        *m_pool,
        vk::CommandBufferLevel::ePrimary,
        bufferCount
    );
    m_buffers = device.getLogical().allocateCommandBuffers(allocInfo);
}

vk::CommandPoolCreateInfo CommandPool::createInfo(uint32_t queueFamilyIndex) {
    return vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        queueFamilyIndex
    );
}
