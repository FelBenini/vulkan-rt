#pragma once
#include "core/Device.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <array>

struct Vertex {
    std::array<float, 2> pos;
    std::array<float, 3> color;
};

class VertexBuffer {
public:
    VertexBuffer(const Device& device, const std::vector<Vertex>& vertices);

    VertexBuffer(const VertexBuffer&)            = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    VertexBuffer(VertexBuffer&&) noexcept = default;
    VertexBuffer& operator=(VertexBuffer&&) noexcept = default;

    const vk::raii::Buffer&       getBuffer()   const { return m_buffer; }
    const vk::raii::DeviceMemory& getMemory()    const { return m_memory; }
    uint32_t                      getCount()    const { return m_count; }

private:
    vk::raii::Buffer       m_buffer  = nullptr;
    vk::raii::DeviceMemory m_memory  = nullptr;
    uint32_t               m_count   = 0;

public:
    VertexBuffer() = default;
};
