#pragma once
#include "vk_defs.h"
#include <vulkan/vulkan_core.h>

#pragma region [ Vertex Defs and Constructor ]
typedef struct 
{
    float x, y, z, r, g, b, u, v;
}
Vertex;

static Vertex new_vertex(   float x, 
                            float y, 
                            float z, 
                            float r, 
                            float g, 
                            float b, 
                            float u, 
                            float v)
{
    return (Vertex){x, y, z, r, g, b, u, v};
}
#pragma endregion
#pragma region [ Vertex Descriptions ]
static void vk_GetVertexDescriptions(   VkVertexInputAttributeDescription* inputAttrDesc, 
                                        uint32_t *size,
                                        VkVertexInputBindingDescription* inputBindDesc)
{
    if (inputAttrDesc == NULL)
    {
        *size = 3;
        return;
    }

    inputAttrDesc[0].binding    = 0;
    inputAttrDesc[0].format     = VK_FORMAT_R32G32B32_SFLOAT;
    inputAttrDesc[0].location   = 0;
    inputAttrDesc[0].offset     = 0*sizeof(float);

    inputAttrDesc[1].binding    = 0;
    inputAttrDesc[1].format     = VK_FORMAT_R32G32B32_SFLOAT;
    inputAttrDesc[1].location   = 1;
    inputAttrDesc[1].offset     = 3*sizeof(float);

    inputAttrDesc[2].binding    = 0;
    inputAttrDesc[2].format     = VK_FORMAT_R32G32_SFLOAT;
    inputAttrDesc[2].location   = 2;
    inputAttrDesc[2].offset     = 6*sizeof(float);

    inputBindDesc->binding      = 0;
    inputBindDesc->inputRate    = VK_VERTEX_INPUT_RATE_VERTEX;
    inputBindDesc->stride       = sizeof(Vertex);
}
#pragma endregion