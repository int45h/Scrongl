#pragma once

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "vk_defs.h"

#pragma region [ Misc Utility Functions ]
// For now, print all available layers
static void vk_PrintValidLayers()
{
    uint32_t layerCount;
    VkLayerProperties* layerProperties;

    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    layerProperties = (VkLayerProperties*)malloc(layerCount*sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties);

    for (int i = 0; i < layerCount; i++)
        printf("Available Layer: %s (%s)\n", 
            layerProperties[i].layerName, 
            layerProperties[i].description);

    free(layerProperties);
}

static void vk_DestroyExtensionsStruct(vkExtensions *extensions)
{
    // Free available extensions
    if (extensions->availableExtensions == NULL)
        return;

    while ((extensions->availableExtCount--) > 0)
    {
        if (extensions->availableExtensions[extensions->availableExtCount] != NULL)
            free(extensions->availableExtensions[extensions->availableExtCount]);
    }
}
#pragma endregion
#pragma region [ Vulkan Extensions ]
// Function to setup all available VK extensions
static void vk_SetupDefaultExtensions(vkExtensions *extensions, SDL_Window *w)
{
    SDL_Vulkan_GetInstanceExtensions(w, &extensions->availableExtCount, NULL);
    extensions->availableExtensions = alloc(extensions->availableExtCount, char*);
    SDL_Vulkan_GetInstanceExtensions(w, &extensions->availableExtCount, (const char**)extensions->availableExtensions);

    //for (int i = 0; i < extensions->availableExtCount; i++)
    //    printf("Available Extension: %s\n", extensions->availableExtensions[i]);
}
#pragma endregion
#pragma region [ Framebuffers, Image Views ]
static VkResult vk_CreateFramebuffers(vkContext *context, vkRenderState *state)
{
    VkFramebufferCreateInfo fbInfo = {};
    VkExtent2D extent;
    VkResult result = VK_ERROR_UNKNOWN;

    extent.width    = context->displayExtent.width;
    extent.height   = context->displayExtent.height;

    state->framebuffers = alloc(state->imageCount, VkFramebuffer);
    for (int i = 0; i < state->imageCount; i++)
    {
        fbInfo.sType            = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.width            = extent.width;
        fbInfo.height           = extent.height;
        fbInfo.layers           = 1;
        fbInfo.attachmentCount  = 1;
        fbInfo.pAttachments     = &state->imageViews[i];
        fbInfo.renderPass       = state->renderPass;

        result = vkCreateFramebuffer(context->device, &fbInfo, NULL, &state->framebuffers[i]);
        VK_CHECK(result, "failed to create framebuffer!");
    }
    return result;
}

static void vk_DestroyFramebuffers( vkContext *context, 
                                    uint32_t fbCount, 
                                    VkFramebuffer *pBuffers)
{
    for (int i = 0; i < fbCount; i++)
        vkDestroyFramebuffer(context->device, pBuffers[i], NULL);
}

static void vk_DestroyImageViews(   vkContext *context, 
                                    uint32_t imgCount, 
                                    VkImageView *pViews)
{
    for (int i = 0; i < imgCount; i++)
        vkDestroyImageView(context->device, pViews[i], NULL);
}
#pragma endregion
#pragma region [ Command Buffers ]
static VkResult vk_CreateCommandPool(vkContext *context, vkRenderState *state, VkCommandPool *cmdPool)
{
    VkCommandPoolCreateInfo cmdPoolInfo = {};

    cmdPoolInfo.sType               = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.flags               = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolInfo.queueFamilyIndex    = state->gfxQueueFamilyIndex;

    return vkCreateCommandPool(
        context->device, 
        &cmdPoolInfo, 
        NULL, 
        cmdPool);
}

static VkResult vk_AllocateCommandBuffers(  vkContext *context,
                                            VkCommandPool cmdPool,
                                            VkCommandBufferLevel level,
                                            uint32_t count,
                                            VkCommandBuffer *buffers)
{
    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    VkResult result = VK_ERROR_UNKNOWN;

    cmdAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool        = cmdPool;
    cmdAllocInfo.level              = level;
    cmdAllocInfo.commandBufferCount = count;

    return vkAllocateCommandBuffers(
        context->device, 
        &cmdAllocInfo, 
        buffers
    );
}

static void vk_FreeCommandBuffers(  vkContext *context, 
                                    VkCommandPool cmdPool, 
                                    uint32_t count, 
                                    VkCommandBuffer *buffers)
{
    vkFreeCommandBuffers(
        context->device, 
        cmdPool, 
        count,
        buffers);
}

static void vk_DestroyCommandPool(vkContext *context, VkCommandPool cmdPool)
{
    vkDestroyCommandPool(context->device, cmdPool, NULL);
}
#pragma endregion
#pragma region [ Pipeline ]
static void vk_DestroyPipeline(vkContext *context, VkPipeline pipeline)
{
    vkDestroyPipeline(context->device, pipeline, NULL);
}
#pragma endregion
#pragma region [ Vulkan Destruction ]
static void vk_DestroyRenderState(vkContext *context, vkRenderState *state)
{
    vk_DestroyFramebuffers(context, state->imageCount, state->framebuffers);
    vk_DestroyImageViews(context, state->imageCount, state->imageViews);

    free(state->framebuffers);
    free(state->imageViews);
    vkDestroyRenderPass(context->device, state->renderPass, NULL);
    vkDestroySwapchainKHR(context->device, state->swapchain, NULL);
    //free(state->imageHandles);
}

static void vk_DestroyVulkan(vkContext *context)
{
    vk_DestroyPhysicalDeviceDatabase(&context->db);

    vkDestroyDevice(context->device, NULL);
    vkDestroySurfaceKHR(context->instance, context->surface, NULL);
    vkDestroyInstance(context->instance, NULL);
}
#pragma endregion