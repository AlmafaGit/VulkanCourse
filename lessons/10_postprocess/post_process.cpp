#include "post_process.h"

#include "wrappers.h"

namespace {
#include "post_process.vert_include.h"
#include "post_process.frag_include.h"
}

static VkPipeline CreatePipeline(const VkDevice         device,
                                 const VkPipelineLayout pipelineLayout,
                                 const VkFormat         colorFormat,
                                 const VkShaderModule   shaderVertex,
                                 const VkShaderModule   shaderFragment)
{

    // shader stages
    VkPipelineShaderStageCreateInfo shaders[] = {
        {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0,
            .stage               = VK_SHADER_STAGE_VERTEX_BIT,
            .module              = shaderVertex,
            .pName               = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0,
            .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module              = shaderFragment,
            .pName               = "main",
            .pSpecializationInfo = nullptr,
        },
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = 0,
        .flags                           = 0,
        .vertexBindingDescriptionCount   = 0u,
        .pVertexBindingDescriptions      = nullptr,
        .vertexAttributeDescriptionCount = 0u,
        .pVertexAttributeDescriptions    = nullptr,
    };

    // input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // viewport info
    VkPipelineViewportStateCreateInfo viewportInfo = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .viewportCount = 1,
        .pViewports    = nullptr,
        .scissorCount  = 1,
        .pScissors     = nullptr,
    };

    // rasterization info
    VkPipelineRasterizationStateCreateInfo rasterizationInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_NONE, // VK_CULL_MODE_FRONT_BIT,
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Disabled
        .depthBiasClamp          = 0.0f, // Disabled
        .depthBiasSlopeFactor    = 0.0f, // Disabled
        .lineWidth               = 1.0f,
    };

    // multisample
    VkPipelineMultisampleStateCreateInfo multisampleInfo = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 0.0f,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    };

    // depth stencil
    // "empty" stencil Op state
    VkStencilOpState emptyStencilOp = {};

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .depthTestEnable       = VK_FALSE,
        .depthWriteEnable      = VK_FALSE,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable     = VK_FALSE,
        .front                 = emptyStencilOp,
        .back                  = emptyStencilOp,
        .minDepthBounds        = 0.0f,
        .maxDepthBounds        = 1.0f,
    };

    // color blend
    VkPipelineColorBlendAttachmentState blendAttachment = {
        .blendEnable = VK_TRUE,
        // as blend is disabled fill these with default values,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        // Important!
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .logicOpEnable = VK_FALSE,          // FALSE,
        .logicOp       = VK_LOGIC_OP_CLEAR, // Disabled
        // Important!
        .attachmentCount = 1,
        .pAttachments    = &blendAttachment,
        .blendConstants  = {1.0f, 1.0f, 1.0f, 1.0f}, // Ignored
    };
    const VkPipelineRenderingCreateInfo renderingInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext                   = nullptr,
        .viewMask                = 0,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    const std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    const VkPipelineDynamicStateCreateInfo dynamicInfo = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = nullptr,
        .flags             = 0u,
        .dynamicStateCount = (uint32_t)dynamicStates.size(),
        .pDynamicStates    = dynamicStates.data(),
    };

    // pipeline create
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &renderingInfo,
        .flags               = 0,
        .stageCount          = 2,
        .pStages             = shaders,
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pTessellationState  = nullptr,
        .pViewportState      = &viewportInfo,
        .pRasterizationState = &rasterizationInfo,
        .pMultisampleState   = &multisampleInfo,
        .pDepthStencilState  = &depthStencilInfo,
        .pColorBlendState    = &colorBlendInfo,
        .pDynamicState       = &dynamicInfo,
        .layout              = pipelineLayout,
        .renderPass          = VK_NULL_HANDLE,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = 0,
    };

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult   result   = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
    (void)result;

    return pipeline;
}

PostProcessPass::PostProcessPass(VkFormat colorFormat, VkExtent2D extent)
    : m_colorFormat(colorFormat)
    , m_extent(extent)
{}

bool PostProcessPass::Create(Context& context) {
    const VkDevice device = context.device();

    const std::vector<VkDescriptorSetLayoutBinding> layoutBindingsBase = {
        VkDescriptorSetLayoutBinding{
            .binding            = 0,
            .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr,
        },
    };

    VkDescriptorSetLayout descSetLayout = context.descriptorPool().createLayout(layoutBindingsBase);

    m_pipelineLayout = CreatePipelineLayout(device, {descSetLayout}, sizeof(PostProcessOptions));
    {
        VkShaderModule shaders[] = {
            CreateShaderModule(device, SPV_post_process_vert, sizeof(SPV_post_process_vert)),
            CreateShaderModule(device, SPV_post_process_frag, sizeof(SPV_post_process_frag)),
        };

        m_pipeline = CreatePipeline(device, m_pipelineLayout, m_colorFormat, shaders[0], shaders[1]);

        vkDestroyShaderModule(device, shaders[0], nullptr);
        vkDestroyShaderModule(device, shaders[1], nullptr);

    }

    m_descSet = context.descriptorPool().createSet(descSetLayout);

    return VK_SUCCESS;
}

void PostProcessPass::Destroy(Context& context) {
    vkDestroyPipeline(context.device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(context.device(), m_pipelineLayout, nullptr);
}

void PostProcessPass::BeginPass(const VkCommandBuffer cmdBuffer, VkImageView colorOutputView) {
    const VkClearValue                 clearColor      = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    const VkRenderingAttachmentInfoKHR colorAttachment = {
        .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .pNext              = nullptr,
        .imageView          = colorOutputView,
        .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode        = VK_RESOLVE_MODE_NONE,
        .resolveImageView   = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue         = clearColor,
    };
    const VkRenderingInfoKHR renderInfo = {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .pNext                = nullptr,
        .flags                = 0,
        .renderArea           = {.offset = {0, 0}, .extent = m_extent},
        .layerCount           = 1,
        .viewMask             = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttachment,
        .pDepthAttachment     = nullptr,
        .pStencilAttachment   = nullptr,
    };
    vkCmdBeginRendering(cmdBuffer, &renderInfo);

    const VkViewport viewport = {
        .x        = 0,
        .y        = 0,
        .width    = float(m_extent.width),
        .height   = float(m_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = m_extent,
    };
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descSet, 0,
                            nullptr);
    vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PostProcessOptions), &options);
}

void PostProcessPass::Draw(const VkCommandBuffer cmdBuffer) {
    vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
}

void PostProcessPass::EndPass(const VkCommandBuffer cmdBuffer) {
    vkCmdEndRendering(cmdBuffer);
}

void PostProcessPass::BindInputImage(const VkDevice device, const Texture& texture) {
    DescriptorSetMgmt descSetMgmt(m_descSet);
    descSetMgmt.SetImage(0, texture.view(), texture.sampler());
    descSetMgmt.Update(device);
}
