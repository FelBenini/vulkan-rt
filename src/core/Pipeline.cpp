#include "core/Pipeline.hpp"
#include "core/VertexBuffer.hpp"
#include "platform/Window.hpp"
#include <fstream>
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Pipeline::Pipeline(const Device& device, const Swapchain& swapchain)
    : m_swapchainRenderPass(&swapchain.getRenderPass())
{
    const auto& dev = device.getLogical();

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
        vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
        vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
    };

    vk::PipelineVertexInputStateCreateInfo vertexInput(
        {},
        1, &bindingDesc,
        static_cast<uint32_t>(vertexAttribs.size()), vertexAttribs.data()
    );

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly(
        {},
        vk::PrimitiveTopology::eTriangleList,
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
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eCounterClockwise,
        false,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    );

    vk::PipelineMultisampleStateCreateInfo multisampling(
        {},
        swapchain.getMsaaSamples(),
        false,
        1.0f,
        nullptr,
        false,
        false
    );

    vk::PipelineDepthStencilStateCreateInfo depthStencil(
        {},
        true,
        true,
        vk::CompareOp::eLess,
        false,
        false,
        {},
        {},
        0.0f,
        1.0f
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

    vk::PushConstantRange pushConstantRange(
        vk::ShaderStageFlagBits::eVertex,
        0,
        sizeof(glm::mat4) * 3 // model + view + proj matrices
    );

    vk::PipelineLayoutCreateInfo layoutInfo(
        {},
        0, nullptr,
        1, &pushConstantRange
    );

    m_pipelineLayout = vk::raii::PipelineLayout(dev, layoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo(
        {},
        stages,
        &vertexInput,
        &inputAssembly,
        nullptr,
        &viewportState,
        &rasterizer,
        &multisampling,
        &depthStencil,
        &colorBlending,
        nullptr,
        *m_pipelineLayout,
        *m_swapchainRenderPass,
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
