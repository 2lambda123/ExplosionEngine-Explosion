//
// Created by John Kindem on 2021/3/30.
//

#include <utility>

#include <Explosion/Driver/Pipeline.h>
#include <Explosion/Driver/Device.h>

namespace {
    VkShaderModule CreateShaderModule(const VkDevice& device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule {};
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module");
        }
        return shaderModule;
    }

    void DestroyShaderModule(const VkDevice& device, const VkShaderModule& shaderModule)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }
}

namespace Explosion {
    Pipeline::Pipeline(Device& device, Pipeline::Config config) : device(device), config(std::move(config)) {}

    Pipeline::~Pipeline() = default;

    void Pipeline::CreatePipeline()
    {
        VkShaderModule vertexShaderModule = CreateShaderModule(device.GetVkDevice(), config.shaderConfig.vertexShaderCode);
        VkShaderModule fragmentShaderModule = CreateShaderModule(device.GetVkDevice(), config.shaderConfig.fragmentShaderCode);

        std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(2);
        shaderStageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStageCreateInfos[0].module = vertexShaderModule;
        shaderStageCreateInfos[0].pName = "VertexShader";
        shaderStageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageCreateInfos[1].module = fragmentShaderModule;
        shaderStageCreateInfos[1].pName = "FragmentShader";

        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
        vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {};
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = false;

        VkViewport viewport {};
        viewport.x = config.viewportScissorsConfig.viewPortX;
        viewport.y = config.viewportScissorsConfig.viewPortY;
        viewport.width = config.viewportScissorsConfig.viewPortWidth;
        viewport.height = config.viewportScissorsConfig.viewPortHeight;
        viewport.minDepth = config.viewportScissorsConfig.viewPortMinDepth;
        viewport.maxDepth = config.viewportScissorsConfig.viewPortMaxDepth;

        VkRect2D scissor {};
        scissor.offset.x = config.viewportScissorsConfig.scissorX;
        scissor.offset.y = config.viewportScissorsConfig.scissorY;
        scissor.extent.width = config.viewportScissorsConfig.scissorWidth;
        scissor.extent.height = config.viewportScissorsConfig.scissorHeight;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo {};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {};
        rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.depthClampEnable = config.rasterizationConfig.depthClamp;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = config.rasterizationConfig.discard;
        rasterizationStateCreateInfo.polygonMode = VkConvert<PolygonMode, VkPolygonMode>(config.rasterizationConfig.polygonMode);
        rasterizationStateCreateInfo.lineWidth = 1.f;
        rasterizationStateCreateInfo.cullMode = config.rasterizationConfig.cullMode;
        rasterizationStateCreateInfo.frontFace = VkConvert<FrontFace, VkFrontFace>(config.rasterizationConfig.frontFace);
        rasterizationStateCreateInfo.depthBiasEnable = false;
        rasterizationStateCreateInfo.depthBiasConstantFactor = 0.f;
        rasterizationStateCreateInfo.depthBiasClamp = 0.f;
        rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.f;

        VkPipelineColorBlendAttachmentState colorBlendAttachmentState {};
        colorBlendAttachmentState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.blendEnable = true;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable = false;
        colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
        colorBlendStateCreateInfo.blendConstants[0] = 0.f;
        colorBlendStateCreateInfo.blendConstants[1] = 0.f;
        colorBlendStateCreateInfo.blendConstants[2] = 0.f;
        colorBlendStateCreateInfo.blendConstants[3] = 0.f;

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout(device.GetVkDevice(), &pipelineLayoutCreateInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout");
        }

        DestroyShaderModule(device.GetVkDevice(), vertexShaderModule);
        DestroyShaderModule(device.GetVkDevice(), fragmentShaderModule);
    }

    void Pipeline::DestroyPipeline()
    {
        vkDestroyPipelineLayout(device.GetVkDevice(), vkPipelineLayout, nullptr);
        // TODO
    }
}