#pragma once
#include "core/Device.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <vector>

class Sync {
public:
    Sync(const Device& device, uint32_t maxFramesInFlight);

    Sync(const Sync&)            = delete;
    Sync& operator=(const Sync&) = delete;

    Sync(Sync&&) noexcept = default;
    Sync& operator=(Sync&&) noexcept = default;

    const vk::raii::Semaphore& getImageAvailableSemaphore(uint32_t frameIndex) const {
        return m_imageAvailableSemaphores[frameIndex];
    }
    const vk::raii::Semaphore& getRenderFinishedSemaphore(uint32_t frameIndex) const {
        return m_renderFinishedSemaphores[frameIndex];
    }
    const vk::raii::Fence& getInFlightFence(uint32_t frameIndex) const {
        return m_inFlightFences[frameIndex];
    }

    uint32_t maxFramesInFlight() const { return m_maxFramesInFlight; }

private:
    uint32_t m_maxFramesInFlight;

    std::vector<vk::raii::Semaphore> m_imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::raii::Fence>     m_inFlightFences;
};
