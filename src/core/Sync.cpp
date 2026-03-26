#include "core/Sync.hpp"
#include "core/Device.hpp"

Sync::Sync(const Device& device, uint32_t maxFramesInFlight)
    : m_maxFramesInFlight(maxFramesInFlight)
    , m_imageAvailableSemaphores()
    , m_renderFinishedSemaphores()
    , m_inFlightFences()
{
    const vk::SemaphoreCreateInfo semaphoreInfo;
    const vk::FenceCreateInfo     fenceInfo(vk::FenceCreateFlagBits::eSignaled);

    for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
        m_imageAvailableSemaphores.push_back(device.getLogical().createSemaphore(semaphoreInfo));
        m_renderFinishedSemaphores.push_back(device.getLogical().createSemaphore(semaphoreInfo));
        m_inFlightFences.push_back(device.getLogical().createFence(fenceInfo));
    }
}
