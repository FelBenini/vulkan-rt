#include "core/Pipeline.hpp"
#include "core/VertexBuffer.hpp"
#include "platform/Window.hpp"
#include <fstream>
#include <cstddef>

Pipeline::Pipeline(const Device& device, const Swapchain& swapchain) {
    const auto& dev = device.getLogical();

    m_renderPass = createRenderPass(dev, swapchain.getFormat());

    const auto vertCode = readShader("shaders/triangle.vert.spv");
    const auto fragCode = readShader("shaders/triangle.frag.spv");

    auto vertModule = createShaderModule(dev, vertCode);
    auto fragModule = createShaderModule(dev, fragCode);

    vk::PipelineShaderStageCreateInfo vertStage(
        {},
        vk::ShaderStageFlagBits::eVertex,
        *vertModule,
        "main"
    );

    vk::PipelineShaderStageCreateInfo fragStage(
        {},
        vk::ShaderStageFlagBits::eFragment,
        *fragModule,
        "main"
    );

    std::array stages = {vertStage, fragStage};

    vk::VertexInputBindingDescription bindingDesc(0, sizeof(Vertex), vk::VertexInputRate::eVertex);

    std::array vertexAttribs = {
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos)),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
    };

    vk::PipelineVertexInputStateCreateInfo vertexInput(
        {},
        1, &bindingDesc,
        static_cast<uint32_t>(vertexAttribs.size()), vertexAttribs.data()
    );

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
        {},
        vk::PrimitiveTopology::eTriangleStrip,
        false
    );

    vk::Viewport viewport(
        0.0f,
        0.0f,
        static_cast<float>(swapchain.getExtent().width),
        static_cast<float>(swapchain.getExtent().height),
        0.0f,
        1.0f
    );

    vk::Rect2D scissor({0, 0}, swapchain.getExtent());

    vk::PipelineViewportStateCreateInfo viewportState(
        {},
        1, &viewport,
        1, &scissor
    );

    vk::PipelineRasterizationStateCreateInfo rasterizer(
        {},
        false,
        false,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eClockwise,
        false,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    );

    vk::PipelineMultisampleStateCreateInfo multisampling(
        {},
        vk::SampleCountFlagBits::e1,
        false,
        1.0f,
        nullptr,
        false,
        false
    );

    vk::PipelineColorBlendAttachmentState colorBlendAttachment(
        false,
        vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    );

    vk::PipelineColorBlendStateCreateInfo colorBlending(
        {},
        false,
        vk::LogicOp::eCopy,
        1, &colorBlendAttachment,
        {0.0f, 0.0f, 0.0f, 0.0f}
    );

    m_pipelineLayout = vk::raii::PipelineLayout(dev, vk::PipelineLayoutCreateInfo({}, 0, nullptr));

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        stages,
        &vertexInput,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        nullptr,
        &colorBlending,
        nullptr,
        *m_pipelineLayout,
        *m_renderPass,
        0
    );

    m_pipeline = vk::raii::Pipeline(dev, nullptr, pipelineInfo);
}

std::vector<char> Pipeline::readShader(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader: " + path);
    }
    size_t size = file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

vk::raii::ShaderModule Pipeline::createShaderModule(const vk::raii::Device& device, const std::vector<char>& code) {
    vk::ShaderModuleCreateInfo createInfo(
        {},
        code.size(),
        reinterpret_cast<const uint32_t*>(code.data())
    );
    return vk::raii::ShaderModule(device, createInfo);
}

vk::raii::RenderPass Pipeline::createRenderPass(const vk::raii::Device& device, vk::Format swapchainFormat) {
    vk::AttachmentDescription colorAttachment(
        {},
        swapchainFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR
    );

    vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass(
        {},
        vk::PipelineBindPoint::eGraphics,
        0, nullptr,
        1, &colorAttachmentRef,
        0, nullptr
    );

    vk::SubpassDependency dependency(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits::eColorAttachmentWrite,
        {}
    );

    vk::RenderPassCreateInfo renderPassInfo(
        {},
        1, &colorAttachment,
        1, &subpass,
        1, &dependency
    );

    return vk::raii::RenderPass(device, renderPassInfo);
}
