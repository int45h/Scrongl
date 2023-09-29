#pragma once
#include "vk_defs.h"

#include "vk_vertex.h"
#include "shaders/test_vsh.h"
#include "shaders/test_fsh.h"

static VkResult vk_CreateShaderModule(  vkContext *context, 
                                        const uint32_t *data,
                                        uint64_t size,
                                        VkShaderModule *module)
{
    VkShaderModuleCreateInfo shaderInfo = {};
    VkResult result = VK_ERROR_UNKNOWN;

    shaderInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = size;
    shaderInfo.pCode    = data;

    return vkCreateShaderModule(
        context->device, 
        &shaderInfo, 
        NULL, 
        module
    );
}

static VkResult vk_CreateShaderStage(   vkContext *context, 
                                        VkShaderStageFlagBits stage,
                                        const uint32_t *data,
                                        uint64_t size,
                                        VkShaderModule *module,
                                        VkPipelineShaderStageCreateInfo *shaderInfo)
{
    VkResult result = VK_ERROR_UNKNOWN;
    
    result = vk_CreateShaderModule(context, data, size, module);
    VK_CHECK(result, "failed to create shader module");

    shaderInfo->sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo->stage               = stage;
    shaderInfo->module              = *module;
    shaderInfo->pName               = "main";
    shaderInfo->pSpecializationInfo = NULL;

    return result;
}