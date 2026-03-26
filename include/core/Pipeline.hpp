#pragma once
#include "core/Device.hpp"
#include "core/Swapchain.hpp"
#include <vulkan/vulkan_raii.hpp>

class Pipeline {
public:
    Pipeline(const Device& device, const Swapchain& swapchain);

    Pipeline(const Pipeline&)            = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    Pipeline(Pipeline&&) noexcept = default;
    Pipeline& operator=(Pipeline&&) noexcept = default;

    const vk::raii::Pipeline&      getPipeline()      const { return m_pipeline; }
    const vk::raii::PipelineLayout& getLayout()       const { return m_pipelineLayout; }
    const vk::raii::RenderPass&    getRenderPass()   const { return m_renderPass; }

private:
    vk::raii::Pipeline       m_pipeline = nullptr;
    vk::raii::PipelineLayout m_pipelineLayout = nullptr;
    vk::raii::RenderPass     m_renderPass = nullptr;

    static std::vector<char> readShader(const std::string& path);
    vk::raii::ShaderModule   createShaderModule(const vk::raii::Device& device, const std::vector<char>& code);
    static vk::raii::RenderPass createRenderPass(const vk::raii::Device& device, vk::Format swapchainFormat);
};
