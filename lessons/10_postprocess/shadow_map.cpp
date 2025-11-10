#include "shadow_map.h"

#include <cassert>

#include "glm_config.h"
#include "wrappers.h"

namespace {
#include "shadow_map.frag_include.h"
#include "shadow_map.vert_include.h"
} // namespace

ShadowMap::ShadowMap(const VkFormat depthFormat, const uint32_t pushConstantStart, VkExtent2D extent)
    : m_depthFormat(depthFormat)
    , m_extent(extent)
    , m_pushConstantStart(pushConstantStart)
{
}

bool ShadowMap::Create(Context& context)
{
    VkPhysicalDevice phyDevice = context.physicalDevice();
    VkDevice         device    = context.device();

    m_shadowDepth = Texture::Create2D(phyDevice, device, m_depthFormat, m_extent,
                                      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    assert(m_shadowDepth.IsValid());

    m_pipelineLayout = CreatePipelineLayout(device, {}, m_pushConstantStart + sizeof(glm::mat4));
    BuildPipeline(device, m_pipelineLayout);

    return true;
}

bool ShadowMap::BuildPipeline(const VkDevice device, const VkPipelineLayout pipelineLayout)
{
    VkShaderModule shaderVertex   = CreateShaderModule(device, SPV_shadow_map_vert, sizeof(SPV_shadow_map_vert));
    VkShaderModule shaderFragment = CreateShaderModule(device, SPV_shadow_map_frag, sizeof(SPV_shadow_map_frag));

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

    // Vertex Infor information must match across all objects!
    // IMPORTANT!
    VkVertexInputBindingDescription vertexBinding = {
        // binding position in Vulkan API, maches the binding in VkVertexInputAttributeDescription
        .binding   = 0,
        .stride    = sizeof(float) * (3 + 2 + 3), // step by "vec3 + vec2 + vec3" elements in the attached buffer
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX, // step "stride" bytes each for vertex in the buffer
    };

    // IMPORTANT!
    VkVertexInputAttributeDescription vertexAttributes[3] = {
        {
            // position
            .location = 0,                          // layout location=0 in shader
            .binding  = 0,                          // binding position in Vulkan API
            .format   = VK_FORMAT_R32G32B32_SFLOAT, // use "vec3" values from the buffer
            .offset   = 0,                          // use buffer from the 0 byte
        },
        {
            // uv
            .location = 1,                       // layout location=1 in shader
            .binding  = 0,                       // binding position in Vulkan API
            .format   = VK_FORMAT_R32G32_SFLOAT, // use "vec2" values from the buffer
            .offset   = sizeof(float) * 3,       // use buffer from the 0 byte
        },
        {
            // normals
            .location = 2,                          // layout location=2 in shader
            .binding  = 0,                          // binding position in Vulkan API
            .format   = VK_FORMAT_R32G32B32_SFLOAT, // use "vec3" values from the buffer
            .offset   = sizeof(float) * (3 + 2),    // use buffer from the 0 byte
        },
    };

    // IMPORTANT! related buffer(s) must be bound before draw via vkCmdBindVertexBuffers
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = 0,
        .flags                           = 0,
        .vertexBindingDescriptionCount   = 1u,
        .pVertexBindingDescriptions      = &vertexBinding,
        .vertexAttributeDescriptionCount = 3u,
        .pVertexAttributeDescriptions    = vertexAttributes,
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
    const VkPipelineViewportStateCreateInfo viewportInfo = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .viewportCount = 1,
        .pViewports    = nullptr, // Dynamic state
        .scissorCount  = 1,
        .pScissors     = nullptr, // Dynamic state
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
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_TRUE,
        .depthBiasConstantFactor = 0.5f,
        .depthBiasClamp          = 0.0f,  // Disabled
        .depthBiasSlopeFactor    = 1.75f, // Disabled
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
    VkStencilOpState emptyStencilOp = {}; // Removed memeber init for space

    VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .depthTestEnable       = VK_TRUE, // Depth test is a must!
        .depthWriteEnable      = VK_TRUE,
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
        .blendEnable = VK_FALSE,
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
        .logicOpEnable = VK_FALSE,
        .logicOp       = VK_LOGIC_OP_CLEAR,
        // Important!
        .attachmentCount = 1,
        .pAttachments    = &blendAttachment,
        .blendConstants  = {1.0f, 1.0f, 1.0f, 1.0f}, // Ignored
    };

    const VkPipelineRenderingCreateInfo renderingInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext                   = nullptr,
        .viewMask                = 0,
        .colorAttachmentCount    = 0,
        .pColorAttachmentFormats = nullptr,
        .depthAttachmentFormat   = m_depthFormat,
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

    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline);

    vkDestroyShaderModule(device, shaderVertex, nullptr);
    vkDestroyShaderModule(device, shaderFragment, nullptr);

    return result == VK_SUCCESS;
}

void ShadowMap::Destroy(Context& context)
{
    VkDevice device = context.device();
    m_shadowDepth.Destroy(device);
    vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
    vkDestroyPipeline(device, m_pipeline, nullptr);
}

void ShadowMap::TransitionForRender(const VkCommandBuffer cmdBuffer)
{
    const VkImageMemoryBarrier2 renderStartBarrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext               = nullptr,
        .srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .srcAccessMask       = VK_ACCESS_2_NONE,
        .dstStageMask        = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        .dstAccessMask       = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout           = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = m_shadowDepth.image(),
        .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
    };
    const VkDependencyInfo startDependency = {
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext                    = 0,
        .dependencyFlags          = 0,
        .memoryBarrierCount       = 0,
        .pMemoryBarriers          = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers    = nullptr,
        .imageMemoryBarrierCount  = 1,
        .pImageMemoryBarriers     = &renderStartBarrier,
    };
    vkCmdPipelineBarrier2(cmdBuffer, &startDependency);
}

void ShadowMap::TransitionForRead(const VkCommandBuffer cmdBuffer)
{
    const VkImageMemoryBarrier2 renderEndBarrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext               = nullptr,
        .srcStageMask        = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        .srcAccessMask       = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstStageMask        = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .dstAccessMask       = VK_ACCESS_2_SHADER_READ_BIT,
        .oldLayout           = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .newLayout           = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = m_shadowDepth.image(),
        .subresourceRange =
            {
                .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
    };
    const VkDependencyInfo endDependency = {
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext                    = 0,
        .dependencyFlags          = 0,
        .memoryBarrierCount       = 0,
        .pMemoryBarriers          = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers    = nullptr,
        .imageMemoryBarrierCount  = 1,
        .pImageMemoryBarriers     = &renderEndBarrier,
    };
    vkCmdPipelineBarrier2(cmdBuffer, &endDependency);
}

void ShadowMap::BeginPass(const VkCommandBuffer cmdBuffer)
{
    TransitionForRender(cmdBuffer);

    // Begin render commands
    const VkClearDepthStencilValue     depthClear      = {1.0f, 0u};
    const VkRenderingAttachmentInfoKHR depthAttachment = {
        .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .pNext              = nullptr,
        .imageView          = m_shadowDepth.view(),
        .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .resolveMode        = VK_RESOLVE_MODE_NONE,
        .resolveImageView   = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue         = {.depthStencil = depthClear},
    };
    const VkRenderingInfoKHR renderInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .renderArea =
            {
                .offset = {0, 0},
                .extent = m_extent,
            },
        .layerCount           = 1,
        .viewMask             = 0,
        .colorAttachmentCount = 0,
        .pColorAttachments    = nullptr, //&colorAttachment,
        .pDepthAttachment     = &depthAttachment,
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

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline());
}

void ShadowMap::updateLightInfo(const VkCommandBuffer cmdBuffer, DirectionalLight& lightInfo){
    vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(lightInfo), &lightInfo);
    //vkCmdPushConstants(cmdBuffer, commonLayout, VK_SHADER_STAGE_ALL, sizeof(cameraData), sizeof(lightData), &lightData);
}

void ShadowMap::EndPass(const VkCommandBuffer cmdBuffer)
{
    vkCmdEndRendering(cmdBuffer);
    TransitionForRead(cmdBuffer);
}
