#include "core/VertexBuffer.hpp"
#include "core/Device.hpp"
#include <cstring>

VertexBuffer::VertexBuffer(const Device& device, const std::vector<Vertex>& vertices)
    : m_count(static_cast<uint32_t>(vertices.size()))
{
    const auto& dev = device.getLogical();

    const vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();

    vk::BufferCreateInfo bufferInfo(
        {},
        bufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer,
        vk::SharingMode::eExclusive
    );

    m_buffer = vk::raii::Buffer(dev, bufferInfo);

    auto memRequirements = m_buffer.getMemoryRequirements();
    auto memProps = device.getPhysical().getMemoryProperties();

    uint32_t memoryTypeIndex = 0;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent))) {
            memoryTypeIndex = i;
            break;
        }
    }

    vk::MemoryAllocateInfo allocInfo(memRequirements.size, memoryTypeIndex);

    m_memory = vk::raii::DeviceMemory(dev, allocInfo);
    m_buffer.bindMemory(*m_memory, 0);

    void* data = m_memory.mapMemory(0, bufferSize);
    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    m_memory.unmapMemory();
}
