#include "crystal.h"

#include <GLFW/glfw3.h>
#include <cstdint>

#include <vector>
#include <vulkan/vulkan_core.h>

#include "buffer.h"
#include "context.h"
#include "descriptors.h"
#include "texture.h"
#include "wrappers.h"

namespace {

#include "crystal.frag_include.h"
#include "crystal.vert_include.h"

struct Vertex {
    float x;
    float y;
    float z;
    // normals;
    float n1;
    float n2;
    float n3;
};

static constexpr float g_crystalVertices[] = {
#include "crystal_vertices.inc"
};

//jobb lenne ha számítanám nem betölteném, de ahhoz számolni kéne, a normálokat is elég volt számolni (és még az se biztos hogy jó mert még nincs shading)
static std::vector<Vertex> buildCrystal(const float* crystalVertices, size_t arraySize)
{
    size_t crystalPerVertexItemCount = 3;

    // Output format: { x, y, z, n1, n2, n3 }
    std::vector<Vertex> result; // 10db vertex

    for( size_t i = 0; i < arraySize; i += crystalPerVertexItemCount ) {
        Vertex vertex = {
            crystalVertices[i],
            crystalVertices[i + 1],
            crystalVertices[i + 2],
            0,
            0,
            0,
        };
        result.push_back(vertex);
    }

    glm::vec3 bottom_vertice(result[0].x, result[0].y, result[0].z);
    glm::vec3 top_vertice(result.back().x, result.back().y, result.back().z);

    // Compute face normals
    for (size_t i = 1; i <= result.size() - 2; i++) {
        size_t next = (i == 8) ? 1 : i + 1;
        glm::vec3 v1(result[i].x, result[i].y, result[i].z);
        glm::vec3 v2(result[next].x, result[next].y, result[next].z);

        glm::vec3 faceNormal1 = glm::normalize(glm::cross(v2 - bottom_vertice, v1 - bottom_vertice));
        glm::vec3 faceNormal2 = glm::normalize(glm::cross(v1 - top_vertice, v2 - top_vertice));

        // Accumulate
        result[0].n1 += faceNormal1.x + faceNormal2.x;
        result[0].n2 += faceNormal1.y + faceNormal2.y;
        result[0].n3 += faceNormal1.z + faceNormal2.z;
        result.back().n1 += faceNormal1.x + faceNormal2.x;
        result.back().n2 += faceNormal1.y + faceNormal2.y;
        result.back().n3 += faceNormal1.z + faceNormal2.z;

        result[i].n1 += faceNormal1.x + faceNormal2.x;
        result[i].n2 += faceNormal1.y + faceNormal2.y;
        result[i].n3 += faceNormal1.z + faceNormal2.z;
        result[next].n1 += faceNormal1.x + faceNormal2.x;
        result[next].n2 += faceNormal1.y + faceNormal2.y;
        result[next].n3 += faceNormal1.z + faceNormal2.z;
    }

    // Normalize all vertex normals
    for (auto& v : result) {
        glm::vec3 n(v.n1, v.n2, v.n3);
        n = glm::normalize(n);
        v.n1 = n.x;
        v.n2 = n.y;
        v.n3 = n.z;
    }

    return result;
}

//lehetne adattag, de ha mashol szamolnek akkor igy konnyebb masolgatni kodot :S
static std::vector<uint32_t> buildIndexList()
{
    std::vector<uint32_t> indexList = {
        //BOTTOM HALF
        0, 2, 1,
        0, 3, 2,
        0, 4, 3,
        0, 5, 4,
        0, 6, 5,
        0, 7, 6,
        0, 8, 7,
        0, 1, 8,

        //TOP HALF
        9, 1, 2,
        9, 2, 3,
        9, 3, 4,
        9, 4, 5,
        9, 5, 6,
        9, 6, 7,
        9, 7, 8,
        9, 8, 1,
    };

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
        //position
        {
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = 0u,
        },
        // Normal inputs
        {
            .location = 1,
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
        .vertexAttributeDescriptionCount = 2u,
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
        .polygonMode             = VK_POLYGON_MODE_LINE,
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

Crystal::Crystal()
    : m_pipelineLayout(VK_NULL_HANDLE)
    , m_pipeline(VK_NULL_HANDLE)
    , m_constantOffset(0)
{
}

VkResult Crystal::Create(Context& context, const VkFormat colorFormat, const uint32_t pushConstantStart)
{
    const VkDevice       device         = context.device();
    const VkShaderModule shaderVertex   = CreateShaderModule(device, SPV_crystal_vert, sizeof(SPV_crystal_vert));
    const VkShaderModule shaderFragment = CreateShaderModule(device, SPV_crystal_frag, sizeof(SPV_crystal_frag));

    m_device = device;

    const std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
        VkDescriptorSetLayoutBinding{
            .binding            = 0,
            .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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
        const std::vector<Vertex> vertexData     = buildCrystal(g_crystalVertices, std::size(g_crystalVertices));
        const uint32_t            vertexDataSize = vertexData.size() * sizeof(vertexData[0]);
        m_vertexBuffer =
            BufferInfo::Create(context.physicalDevice(), device, vertexDataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        m_vertexBuffer.Update(device, vertexData.data(), vertexDataSize);
    }

    {
        const std::vector<uint32_t> indexData     = buildIndexList();
        const uint32_t              indexDataSize = indexData.size() * sizeof(indexData[0]);
        m_indexBuffer =
            BufferInfo::Create(context.physicalDevice(), device, indexDataSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        m_indexBuffer.Update(device, indexData.data(), indexDataSize);
        m_vertexCount = indexData.size();
    }

    m_uniformBuffer = BufferInfo::Create(context.physicalDevice(), device, sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    const UniformBuffer data = {
        .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        .time  = (float)glfwGetTime(),
    };
    m_uniformBuffer.Update(device, &data, sizeof(data));

    m_modelSet = context.descriptorPool().createSet(m_descSetLayout);

    DescriptorSetMgmt setMgmt(m_modelSet);
    setMgmt.SetBuffer(0, m_uniformBuffer.buffer);
    setMgmt.Update(device);

    return VK_SUCCESS;
}

void Crystal::Destroy(Context& context)
{
    const VkDevice device = context.device();

    m_uniformBuffer.Destroy(device);
    m_vertexBuffer.Destroy(device);
    m_indexBuffer.Destroy(device);
    vkDestroyPipeline(device, m_pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
}

void Crystal::Draw(const VkCommandBuffer cmdBuffer, bool bindPipeline)
{
    const UniformBuffer data = {
        .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        .time  = (float)glfwGetTime(),
    };
    m_uniformBuffer.Update(m_device, &data, sizeof(data));

    ModelPushConstant modelData = {
        .model = glm::mat4(1.0f) * m_position * m_rotation,
    };

    modelData.model = glm::rotate( glm::translate(modelData.model, glm::vec3(0.0f, 1.0f, 0.0f)), data.time, glm::vec3(0.0f, 1.0f, 0.0f));


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
