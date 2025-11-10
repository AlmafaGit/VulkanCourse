#include "grid.h"

#include <GLFW/glfw3.h>
#include <cstdio>

#include <vector>
#include <vulkan/vulkan_core.h>

#include "buffer.h"
#include "context.h"
#include "descriptors.h"
#include "texture.h"
#include "wrappers.h"

namespace {
#include "grid.frag_include.h"
#include "grid.vert_include.h"

struct Vertex {
    float x;
    float y;
    float z;
    float u;
    float v;
    // normals;
    float n1;
    float n2;
    float n3;
};

static std::vector<Vertex> buildGrid(float width, float height, uint32_t count)
{
    // Output format: { x, y, z, u, v }
    std::vector<Vertex> result;

    float halfWidth  = width / 2.0f;
    float halfHeight = height / 2.0f;

    for (uint32_t y = 0; y <= count; y++) {
        for (uint32_t x = 0; x <= count; x++) {
            Vertex vertex = {
                (float)x / count * width - halfWidth,
                (float)y / count * height - halfHeight,
                0.0f,
                (float)x / count,
                (float)y / count,
                0.0f,
                0.0f,
                -1.0f,
            };

            result.push_back(vertex);
        }
    }

    return result;
}

static std::vector<uint32_t> buildIndexList(uint32_t splitCount)
{
    std::vector<uint32_t> indexList;

    for (uint32_t y = 0; y < splitCount; y++) {
        for (uint32_t x = 0; x < splitCount; x++) {
            uint32_t row     = y * (splitCount + 1);
            uint32_t rowNext = (y + 1) * (splitCount + 1);

            uint32_t triangleIndices[] = {
                // triangle 1
                row + x,
                rowNext + x + 1,
                row + x + 1,

                // triangle 2
                row + x,
                rowNext + x,
                rowNext + x + 1,
            };

            indexList.insert(indexList.end(), triangleIndices, triangleIndices + 6);
        }
    }

    return indexList;
}

VkPipeline CreateSimplePipeline(const VkDevice         device,
                                const VkFormat         colorFormat,
                                const VkPipelineLayout pipelineLayout,
                                const VkShaderModule   shaderVertex,
                                const VkShaderModule   shaderFragment)
{
    // shader stages
    const VkPipelineShaderStageCreateInfo shaders[] = {
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

    const VkVertexInputBindingDescription bindingInfo = {
        .binding   = 0,
        .stride    = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    const VkVertexInputAttributeDescription attributeInfos[] = {
        {
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = 0u,
        },
        {
            .location = 1,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32_SFLOAT,
            .offset   = offsetof(Vertex, u),
        },
        // Normal inputs
        {
            .location = 2,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(Vertex, n1),
        },
    };

    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = 0,
        .flags                           = 0,
        .vertexBindingDescriptionCount   = 1u,
        .pVertexBindingDescriptions      = &bindingInfo,
        .vertexAttributeDescriptionCount = 3u,
        .pVertexAttributeDescriptions    = attributeInfos,
    };

    // input assembly
    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
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
    // Experiments:
    //  1) Switch polygon mode to "wireframe".
    //  2) Switch cull mode to front.
    //  3) Switch front face mode to clockwise.
    const VkPipelineRasterizationStateCreateInfo rasterizationInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_NONE,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Disabled
        .depthBiasClamp          = 0.0f, // Disabled
        .depthBiasSlopeFactor    = 0.0f, // Disabled
        .lineWidth               = 1.0f,
    };

    // multisample
    const VkPipelineMultisampleStateCreateInfo multisampleInfo = {
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
    const VkStencilOpState emptyStencilOp = {
        .failOp      = VK_STENCIL_OP_KEEP,
        .passOp      = VK_STENCIL_OP_KEEP,
        .depthFailOp = VK_STENCIL_OP_KEEP,
        .compareOp   = VK_COMPARE_OP_NEVER,
        .compareMask = 0,
        .writeMask   = 0,
        .reference   = 0,
    };

    const VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .depthTestEnable       = VK_TRUE,
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
    const VkPipelineColorBlendAttachmentState blendAttachment = {
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

    const VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .logicOpEnable = VK_FALSE,
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
        .depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT,
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
    const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
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
    assert(result == VK_SUCCESS);

    return pipeline;
}

} // anonymous namespace

Grid::Grid()
    : m_pipelineLayout(VK_NULL_HANDLE)
    , m_pipeline(VK_NULL_HANDLE)
    , m_constantOffset(0)
{
}

VkResult Grid::Create(Context& context,
                      const VkFormat colorFormat,
                      const uint32_t pushConstantStart,
                      float          width,
                      float          height,
                      uint32_t       count)
{
    const VkDevice       device         = context.device();
    const VkShaderModule shaderVertex   = CreateShaderModule(device, SPV_grid_vert, sizeof(SPV_grid_vert));
    const VkShaderModule shaderFragment = CreateShaderModule(device, SPV_grid_frag, sizeof(SPV_grid_frag));

    m_device = device;

    const std::string imagePath = "./images/checker-map_tho.png";
    m_texture = *Texture::LoadFromFile(context.physicalDevice(), device, context.queue(), context.commandPool(),
                                       imagePath, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

    const std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
        VkDescriptorSetLayoutBinding{
            .binding            = 0,
            .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr,
        },
        VkDescriptorSetLayoutBinding{
            .binding            = 1,
            .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr,
        },
    };

    m_descSetLayout = context.descriptorPool().createLayout(layoutBindings);

    m_constantOffset = pushConstantStart;
    m_pipelineLayout = CreatePipelineLayout(device, {m_descSetLayout}, m_constantOffset + sizeof(ModelPushConstant));
    m_pipeline       = CreateSimplePipeline(device, colorFormat, m_pipelineLayout, shaderVertex, shaderFragment);

    vkDestroyShaderModule(device, shaderVertex, nullptr);
    vkDestroyShaderModule(device, shaderFragment, nullptr);

    {
        const std::vector<Vertex> vertexData     = buildGrid(width, height, count);
        const uint32_t            vertexDataSize = vertexData.size() * sizeof(vertexData[0]);
        m_vertexBuffer =
            BufferInfo::Create(context.physicalDevice(), device, vertexDataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        m_vertexBuffer.Update(device, vertexData.data(), vertexDataSize);
    }

    {
        const std::vector<uint32_t> indexData     = buildIndexList(count);
        const uint32_t              indexDataSize = indexData.size() * sizeof(indexData[0]);
        m_indexBuffer =
            BufferInfo::Create(context.physicalDevice(), device, indexDataSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        m_indexBuffer.Update(device, indexData.data(), indexDataSize);
        m_vertexCount = indexData.size();
    }

    m_uniformBuffer =
        BufferInfo::Create(context.physicalDevice(), device, sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    const UniformBuffer data = {
        .color = glm::vec4(1.0f, 0.2f, 1.0f, 1.0f),
        .time  = (float)glfwGetTime(),
    };
    m_uniformBuffer.Update(device, &data, sizeof(data));

    m_modelSet = context.descriptorPool().createSet(m_descSetLayout);

    DescriptorSetMgmt setMgmt(m_modelSet);
    setMgmt.SetBuffer(0, m_uniformBuffer.buffer);
    setMgmt.SetImage(1, m_texture.view(), m_texture.sampler());
    setMgmt.Update(device);

    return VK_SUCCESS;
}

void Grid::Destroy(Context& context)
{
    const VkDevice device = context.device();

    //context.descriptorPool().destroySet(m_modelSet);

    m_texture.Destroy(device);
    m_uniformBuffer.Destroy(device);
    m_vertexBuffer.Destroy(device);
    m_indexBuffer.Destroy(device);
    vkDestroyPipeline(device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
}

void Grid::Draw(const VkCommandBuffer cmdBuffer, bool bindPipeline)
{
    const UniformBuffer data = {
        .color = glm::vec4(1.0f, 0.2f, 1.0f, 1.0f),
        .time  = (float)glfwGetTime(),
    };
    m_uniformBuffer.Update(m_device, &data, sizeof(data));

    ModelPushConstant modelData = {
        .model = glm::mat4(1.0f) * m_position * m_rotation,
    };

    if (bindPipeline) {
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    }
    vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_ALL, m_constantOffset,
                       sizeof(ModelPushConstant), &modelData);

    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_modelSet, 0,
                            nullptr);
    VkDeviceSize nullOffset = 0u;
    vkCmdBindVertexBuffers(cmdBuffer, 0u, 1u, &m_vertexBuffer.buffer, &nullOffset);
    vkCmdBindIndexBuffer(cmdBuffer, m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmdBuffer, m_vertexCount, 1, 0, 0, 0);
}
