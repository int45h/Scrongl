#pragma once
#include "vk_common.h"
#include "vk_defs.h"
#include "vk_error.h"
#include "vk_shaders.h"
#include "vk_vertex.h"

#include <SDL2/SDL_error.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#define arr_size(x, t) sizeof(x) / sizeof(t)

#pragma region [ Predefined Layers and Extensions ]
static const char* layers[] = {
    "VK_LAYER_KHRONOS_validation",
    //"VK_LAYER_RENDERDOC_Capture"
};

static const char* extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
#pragma endregion
#pragma region [ Instance, Surface, Device ]
static VkResult vk_CreateDefaultInstanceAndSurface( vkContext *context, 
                                                    SDL_Window* window)
{ 
    VkResult        result;
    vkExtensions    extensions;
    
    // Set application info
    VkApplicationInfo appInfo   = {};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion          = VK_API_VERSION_1_2;
    appInfo.pApplicationName    = "cumulous";
    appInfo.pEngineName         = "vktest";
    appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pNext               = NULL;
    appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);

    // Get all extensions
    vk_PrintValidLayers();
    vk_SetupDefaultExtensions(&extensions, window);

    // Set instance info
    VkInstanceCreateInfo instanceInfo       = {};
    instanceInfo.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo           = &appInfo;    
    instanceInfo.enabledExtensionCount      = extensions.availableExtCount;
    instanceInfo.ppEnabledExtensionNames    = (const char**)extensions.availableExtensions;
    instanceInfo.enabledLayerCount          = 1;
    instanceInfo.ppEnabledLayerNames        = layers;
    instanceInfo.flags                      = 0;
    instanceInfo.pNext                      = NULL;

    // Create the vulkan instance
    result = vkCreateInstance(&instanceInfo, NULL, &context->instance);
    VK_CHECK(result, "failed to create vulkan instance!");
    
    // Create vulkan surface
    if (!SDL_Vulkan_CreateSurface(window, context->instance, &context->surface))
    {
        fprintf(stderr, "error: %s\n", SDL_GetError());
        return result = VK_ERROR_SURFACE_LOST_KHR;
    }

    return result;
}
static VkResult vk_GetLogicalDevice(vkContext *context)
{
    VkDeviceCreateInfo      deviceInfo = {};
    VkDeviceQueueCreateInfo queueInfo = {};
    float queuePriority = 1.0f;
    VkResult result = VK_ERROR_UNKNOWN;

    vk_BuildPhysicalDeviceDatabase(context, &context->db);
    
    char *devName = context->db.devProperties[context->db.deviceIndex].deviceName;
    printf("\nSelected device: %s\nQueue family: %d (queues: %d)\nAvailable present modes: \n", 
        devName,
        context->db.queueIndex,
        context->db.queueFamilies[context->db.deviceIndex][context->db.queueIndex].queueCount);
    
    for (int pIndex = 0; pIndex < context->db.presentFmtCount[context->db.deviceIndex]; pIndex++)
        printf("\t%s\n", VkPresentModeToString(context->db.presentFormats[context->db.deviceIndex][pIndex]));

    printf("Available formats and color spaces\n");
    for (int fIndex = 0; fIndex < context->db.surfaceFmtCount[context->db.deviceIndex]; fIndex++)
        printf("\t%s (%s)\n", 
            VkFormatToString(context->db.surfaceFormats[context->db.deviceIndex][fIndex].format),
            VkColorSpaceToString(context->db.surfaceFormats[context->db.deviceIndex][fIndex].colorSpace));
    
    // Create the device queue
    queueInfo.sType             = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueCount        = 1;
    queueInfo.queueFamilyIndex  = context->db.queueIndex;
    queueInfo.pQueuePriorities  = &queuePriority;

    // Create the device info struct
    deviceInfo.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos        = &queueInfo;
    deviceInfo.queueCreateInfoCount     = 1;
    deviceInfo.ppEnabledLayerNames      = layers;
    deviceInfo.enabledLayerCount        = 1;
    deviceInfo.ppEnabledExtensionNames  = extensions;
    deviceInfo.enabledExtensionCount    = 1;

    result = vkCreateDevice(
        context->db.devices[context->db.deviceIndex], 
        &deviceInfo, 
        NULL, 
        &context->device
    );
    
    VK_CHECK(result, "failed to create device!");

    return result;
}
#pragma endregion
#pragma region [ Swapchain, Image Views ]
static VkResult vk_CreateDefaultSwapchain(  vkContext *context, 
                                            vkRenderState *state, 
                                            uint32_t w, 
                                            uint32_t h)
{
    VkSurfaceCapabilitiesKHR    surfaceCaps;
    VkSwapchainCreateInfoKHR    swapchainInfo = {};
    VkResult result = VK_ERROR_UNKNOWN;

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->activeDevice, context->surface, &surfaceCaps);
    VK_CHECK(result, "cannot get surface capbilities!");

    context->displayExtent = surfaceCaps.currentExtent;

    vkGetDeviceQueue(context->device, state->gfxQueueFamilyIndex, 0, &state->queue);
    
    printf("\nActive format: %s\nActive color space: %s\nNumber of images (MAX): %d\nName: %s\n", 
        VkFormatToString(state->activeFmt.format), 
        VkColorSpaceToString(state->activeFmt.colorSpace),
        surfaceCaps.minImageCount,
        state->activeDevProperties.deviceName
    );

    swapchainInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface               = context->surface;
    swapchainInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageFormat           = state->activeFmt.format;
    swapchainInfo.imageColorSpace       = state->activeFmt.colorSpace;
    swapchainInfo.imageExtent           = (VkExtent2D){w, h};
    swapchainInfo.clipped               = VK_TRUE;
    swapchainInfo.oldSwapchain          = NULL;
    swapchainInfo.imageArrayLayers      = 1;
    swapchainInfo.minImageCount         = 2;
    swapchainInfo.preTransform          = surfaceCaps.currentTransform;
    swapchainInfo.presentMode           = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices   = NULL;

    result = vkCreateSwapchainKHR(
        context->device, 
        &swapchainInfo, 
        NULL, 
        &state->swapchain
    );
    VK_CHECK(result, "failed to create swapchain!");

    // Get swapchain images
    vkGetSwapchainImagesKHR(context->device, state->swapchain, &state->imageCount, NULL);
    state->imageHandles = alloc(state->imageCount, VkImage);
    vkGetSwapchainImagesKHR(context->device, state->swapchain, &state->imageCount, state->imageHandles);

    return result;
}

static VkResult vk_CreateImageViews(vkContext *context, 
                                    vkRenderState *state)
{
    VkImageViewCreateInfo   imageViewInfo = {};
    VkImageSubresourceRange imageViewSubResource = {};
    VkResult result = VK_ERROR_UNKNOWN;

    // Allocate image views
    state->imageViews = alloc(state->imageCount, VkImageView);
    for (int i = 0; i < state->imageCount; i++)
    {
        // Setup subresource of the image
        imageViewSubResource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewSubResource.layerCount     = 1;
        imageViewSubResource.levelCount     = 1;
        imageViewSubResource.baseArrayLayer = 0;
        imageViewSubResource.baseMipLevel   = 0;

        // Setup image views
        imageViewInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.viewType              = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format                = state->activeFmt.format;
        imageViewInfo.image                 = state->imageHandles[i];
        imageViewInfo.components.r          = VK_COMPONENT_SWIZZLE_R;
        imageViewInfo.components.g          = VK_COMPONENT_SWIZZLE_G;
        imageViewInfo.components.b          = VK_COMPONENT_SWIZZLE_B;
        imageViewInfo.components.a          = VK_COMPONENT_SWIZZLE_A;
        imageViewInfo.subresourceRange      = imageViewSubResource;

        // Create the image view
        result = vkCreateImageView(  
            context->device, 
            &imageViewInfo, 
            NULL, 
            &state->imageViews[i]);

        VK_CHECK(result, "failed to create image view!");
    }

    return result;
}
#pragma endregion
#pragma region [ Subpasses ]
static void vk_CreateColorSubpass(  VkFormat imageFmt,
                                    VkAttachmentDescription *colorAttachmentDesc,
                                    VkAttachmentReference *colorAttachmentRef,
                                    VkSubpassDescription *colorSubpass)
{
    colorAttachmentDesc->initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDesc->finalLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachmentDesc->format          = imageFmt;
    colorAttachmentDesc->loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDesc->storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDesc->stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDesc->stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDesc->samples         = VK_SAMPLE_COUNT_1_BIT;

    colorAttachmentRef->attachment       = 0;
    colorAttachmentRef->layout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    colorSubpass->pipelineBindPoint      = VK_PIPELINE_BIND_POINT_GRAPHICS;
    colorSubpass->colorAttachmentCount   = 1;
    colorSubpass->pColorAttachments      = colorAttachmentRef;
}
#pragma endregion
#pragma region [ Render Pass ]
static VkResult vk_CreateRenderPass(vkContext *context, 
                                    vkRenderState *state)
{
    VkRenderPassCreateInfo  renderPassInfo      = {};
    VkSubpassDescription    colorSubpass        = {};
    VkAttachmentDescription colorAttachmentDesc = {};
    VkAttachmentReference   colorAttachmentRef  = {};
    VkResult result = VK_ERROR_UNKNOWN;

    // Create color attachment subpass
    vk_CreateColorSubpass(
        state->activeFmt.format, 
        &colorAttachmentDesc, 
        &colorAttachmentRef, 
        &colorSubpass
    );

    // Create the render pass
    renderPassInfo.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.subpassCount         = 1;
    renderPassInfo.pSubpasses           = &colorSubpass;
    renderPassInfo.attachmentCount      = 1;
    renderPassInfo.pAttachments         = &colorAttachmentDesc;

    result = vkCreateRenderPass(
        context->device,
        &renderPassInfo,
        NULL,
        &state->renderPass
    );
    VK_CHECK(result, "failed to create render pass!");
    return result;
}
#pragma endregion
#pragma region [ Graphics Pipeline ]
static VkResult vk_CreateDefaultGraphicsPipeline(   vkContext *context, 
                                                    vkRenderState *state, 
                                                    uint32_t layoutIndex,
                                                    VkPushConstantRange *ranges,
                                                    uint32_t rangeCount,
                                                    VkDescriptorSetLayout *layouts,
                                                    uint32_t layoutCount)
{
    VkPipelineDynamicStateCreateInfo        dynamicInfo         = {};
    VkPipelineViewportStateCreateInfo       viewInfo            = {};
    VkPipelineVertexInputStateCreateInfo    vinputInfo          = {};
    VkPipelineInputAssemblyStateCreateInfo  assemblyInfo        = {};
    VkPipelineRasterizationStateCreateInfo  rasterInfo          = {};
    VkPipelineColorBlendStateCreateInfo     colorBlendInfo      = {};
    VkPipelineMultisampleStateCreateInfo    multisampleInfo     = {};
    VkPipelineShaderStageCreateInfo         shaderInfo[2]       = {};

    VkDynamicState                          dynamicStates[2]    = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkVertexInputAttributeDescription       inputAttrDesc[3]    = {};
    VkVertexInputBindingDescription         inputBindDesc       = {};
    uint32_t inputAttrSize = 3;

    VkPipelineColorBlendAttachmentState     attachmentState     = {};

    VkShaderModule                  vsh, fsh;

    VkGraphicsPipelineCreateInfo    gfxInfo     = {};
    VkPipelineLayoutCreateInfo      layoutInfo  = {};
    VkResult result = VK_ERROR_UNKNOWN;
    

    // Shaders
    result = vk_CreateShaderStage(
        context, 
        VK_SHADER_STAGE_VERTEX_BIT, 
        test_vsh_spirv, 
        sizeof(test_vsh_spirv), 
        &vsh, 
        &shaderInfo[0]
    );
    VK_CHECK(result, "failed to create vertex shader");
    vk_CreateShaderStage(
        context, 
        VK_SHADER_STAGE_FRAGMENT_BIT, 
        test_fsh_spirv, 
        sizeof(test_fsh_spirv), 
        &fsh, 
        &shaderInfo[1]
    );
    VK_CHECK(result, "failed to create fragment shader");

    // Dynamic info
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.dynamicStateCount   = 2;
    dynamicInfo.pDynamicStates      = dynamicStates;

    // Viewport info
    state->viewport.minDepth    = 0.0f;
    state->viewport.maxDepth    = 1.0f;
    state->viewport.width       = context->displayExtent.width;
    state->viewport.height      = context->displayExtent.height;
    state->viewport.x           = 0;
    state->viewport.y           = 0;

    state->scissor.extent       = context->displayExtent;
    state->scissor.offset       = (VkOffset2D){0,0};

    viewInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewInfo.viewportCount  = 1;
    viewInfo.pViewports     = &state->viewport;
    viewInfo.scissorCount   = 1;
    viewInfo.pScissors      = &state->scissor;

    // Vertex input
    vk_GetVertexDescriptions(inputAttrDesc, &inputAttrSize, &inputBindDesc);
    vinputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vinputInfo.vertexAttributeDescriptionCount  = 3;
    vinputInfo.pVertexAttributeDescriptions     = inputAttrDesc;
    vinputInfo.vertexBindingDescriptionCount    = 1;
    vinputInfo.pVertexBindingDescriptions       = &inputBindDesc;
    
    // Input assembly
    assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assemblyInfo.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assemblyInfo.primitiveRestartEnable = VK_FALSE;

    // Rasterization
    rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterInfo.depthBiasEnable          = VK_FALSE;
    rasterInfo.depthBiasConstantFactor  = 0.0f;
    rasterInfo.depthBiasSlopeFactor     = 0.0f;
    rasterInfo.depthClampEnable         = VK_FALSE;
    rasterInfo.depthBiasClamp           = 0.0f;
    rasterInfo.lineWidth                = 1.0f;
    rasterInfo.rasterizerDiscardEnable  = VK_FALSE;
    rasterInfo.cullMode                 = VK_CULL_MODE_NONE;//VK_CULL_MODE_BACK_BIT;
    rasterInfo.polygonMode              = VK_POLYGON_MODE_FILL;
    rasterInfo.frontFace                = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    
    // Color blending
    attachmentState.colorBlendOp        = VK_BLEND_OP_ADD;
    attachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    attachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachmentState.alphaBlendOp        = VK_BLEND_OP_ADD;
    attachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    attachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    attachmentState.blendEnable         = VK_FALSE;
    attachmentState.colorWriteMask      =   VK_COLOR_COMPONENT_R_BIT | 
                                            VK_COLOR_COMPONENT_G_BIT | 
                                            VK_COLOR_COMPONENT_B_BIT | 
                                            VK_COLOR_COMPONENT_A_BIT;

    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.attachmentCount      = 1;
    colorBlendInfo.pAttachments         = &attachmentState;
    colorBlendInfo.logicOpEnable        = VK_FALSE;
    colorBlendInfo.logicOp              = VK_LOGIC_OP_COPY;
    colorBlendInfo.blendConstants[0]    = 0.0f;
    colorBlendInfo.blendConstants[1]    = 0.0f;
    colorBlendInfo.blendConstants[2]    = 0.0f;
    colorBlendInfo.blendConstants[3]    = 0.0f;

    // Multisample
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.sampleShadingEnable     = VK_FALSE;
    multisampleInfo.rasterizationSamples    = VK_SAMPLE_COUNT_1_BIT;

    // HERE COMES THE BOYYYY
    gfxInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gfxInfo.basePipelineHandle  = VK_NULL_HANDLE;
    gfxInfo.basePipelineIndex   = -1;

    gfxInfo.stageCount          = 2;
    gfxInfo.pStages             = shaderInfo;

    gfxInfo.pViewportState      = &viewInfo;
    gfxInfo.pDynamicState       = &dynamicInfo;
    gfxInfo.pVertexInputState   = &vinputInfo;
    gfxInfo.pInputAssemblyState = &assemblyInfo;
    gfxInfo.pTessellationState  = NULL;
    gfxInfo.pRasterizationState = &rasterInfo;
    gfxInfo.pColorBlendState    = &colorBlendInfo;
    gfxInfo.pDepthStencilState  = NULL;
    gfxInfo.pMultisampleState   = &multisampleInfo;
    gfxInfo.renderPass          = state->renderPass;
    gfxInfo.subpass             = 0;

    // Layout info
    layoutInfo.sType                    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount   = rangeCount;
    layoutInfo.pPushConstantRanges      = ranges;
    layoutInfo.setLayoutCount           = layoutCount;
    layoutInfo.pSetLayouts              = layouts;

    result = vkCreatePipelineLayout(
        context->device, 
        &layoutInfo, 
        NULL, 
        &state->layouts[layoutIndex]
    );
    VK_CHECK(result, "failed to create pipeline layout");

    gfxInfo.layout = state->layouts[layoutIndex];

    result = vkCreateGraphicsPipelines(
        context->device, 
        VK_NULL_HANDLE, 
        1, 
        &gfxInfo, 
        NULL, 
        &state->pipelines[layoutIndex]
    );
    VK_CHECK(result, "failed to create graphics pipeline");

    vkDestroyShaderModule(context->device, vsh, NULL);
    vkDestroyShaderModule(context->device, fsh, NULL);

    return result;
}
#pragma endregion