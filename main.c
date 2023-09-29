#include <SDL2/SDL_events.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <vulkan/vulkan_core.h>
#include "vk_allocator.h"
#include "vk_common.h"
#include "vk_defs.h"
#include "vk_display.h"
#include "common.h"
#include "vk_init.h"
//#include "vk_mem_alloc.h"
#include "vk_memory.h"
#include "vk_vertex.h"
VkCommandPool   cmdPool;
VkCommandBuffer cmdBuffer;

VkSemaphore acquireImage,
            renderFinished;
VkFence     frameInFlight;

uint32_t    imageIndex = 0;

#pragma region [ Memory testing (REMOVE THIS) ]
ScVkAllocator   allocator;
ScVkMemoryBlock vtxBlock,
                idxBlock;

Vertex quad[] = {
    (Vertex){+0.5,+0.5,+0.0, +1.0,+0.0,+0.0, +0.0,+0.0},
    (Vertex){+0.5,-0.5,+0.0, +0.0,+1.0,+0.0, +0.0,+0.0},
    (Vertex){-0.5,-0.5,+0.0, +0.0,+0.0,+1.0, +0.0,+0.0},
    (Vertex){-0.5,+0.5,+0.0, +0.0,+0.0,+1.0, +0.0,+0.0}
};
uint32_t indices[] = {
    0,1,3,
    1,2,3
};

VkDeviceSize mSize = sizeof(quad);
VkDeviceSize iSize = sizeof(indices);

VkDeviceSize offsets[] = {0};
#pragma endregion
#pragma region [ Uniform Buffer testing (REMOVE THIS) ]
ScVkMemoryBlock UBO_memory[2];
VkDescriptorPool descriptorPool;
int frameCount = 2;

// UBO data definition
typedef struct 
{
    uint32_t        m_binding,
                    m_set;
    ScVkMemoryBlock m_uniforms;
} 
ScVkUBO;

VkDescriptorSetLayout   UBO_layout;
VkDescriptorSet         UBO_set;

// DATA SENT TO SHADER (dont make this global in the future)
float modelMatrix[16] = {
    +1.0f,+0.0f,+0.0f,+0.0f,
    +0.0f,+1.0f,+0.0f,+0.0f,
    +0.0f,+0.0f,+1.0f,+0.0f,
    +0.2f,+0.2f,+0.0f,+1.0f,
};
float time = 0.0f;
#pragma endregion
#pragma region [ Memory Testing functions (REMOVE THIS) ]
void reserve_memory(vkDisplay *d)
{
    //printf("Device Type: %d\n", d->renderState.activeDevProperties.deviceType);
    //vk_PrintPhysicalDeviceMemoryProperties(d->context.db.activeDevice);
    VkResult result = VK_ERROR_UNKNOWN;

    // Allocate memory for vertex buffers
    result = vk_InitGlobalAllocator(d->context.device, &d->renderState, 16384, &allocator);
    VK_CHECK_VOID(result, "failed to create allocator");

    result = vk_AllocateMemory(
        d->context.device, 
        mSize, 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        &vtxBlock, 
        &allocator
    );
    VK_CHECK_VOID(result, "failed to allocate vertex buffer");
    result = vk_UploadToDeviceMemory(quad, mSize, vtxBlock, &allocator);
    VK_CHECK_VOID(result, "failed to copy memory to vertex buffer");
    
    // Allocate memory for index buffers
    result = vk_AllocateMemory(
        d->context.device, 
        iSize, 
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        &idxBlock, 
        &allocator
    );
    VK_CHECK_VOID(result, "failed to allocate index buffer");
    result = vk_UploadToDeviceMemory(indices, iSize, idxBlock, &allocator);
    VK_CHECK_VOID(result, "failed to copy memory to index buffer");
    
    //scVkMemory_ArenaAlloc(VkDeviceSize size, ScVkBufferArena *pArena, VkDeviceSize *ptr)
}

void clean_memory(vkDisplay *d)
{
    vk_FreeMemory(vtxBlock, &allocator);
    vk_FreeMemory(idxBlock, &allocator);

    vk_DestroyGlobalAllocator(&d->context, &allocator);
}
#pragma endregion
#pragma region [ Uniform Buffer functions (REMOVE THIS) ]
VkResult init_descriptor_pool(vkDisplay *d)
{
    VkDescriptorPoolCreateInfo poolInfo = {};
    VkDescriptorPoolSize poolSize = {};

    VkResult result = VK_ERROR_UNKNOWN;

    poolSize.type               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount    = 1;

    poolInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount  = 1;
    poolInfo.pPoolSizes     = &poolSize;
    poolInfo.maxSets        = 1;
    poolInfo.flags          = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

    return vkCreateDescriptorPool(
        d->context.device,
        &poolInfo, 
        NULL, 
        &descriptorPool
    );
}

VkResult create_UBO(vkDisplay *d, 
                    uint32_t set, 
                    uint32_t binding, 
                    uint32_t size)
{
    VkDescriptorSetAllocateInfo     UBO_setAllocInfo        = {};
    VkDescriptorSetLayoutCreateInfo UBO_layoutCreateInfo    = {};
    VkDescriptorSetLayoutBinding    UBO_binding             = {};

    VkResult result = VK_ERROR_UNKNOWN;
    
    // Create UBO memory
    for (int i = 0; i < frameCount; i++)
    {
        result = vk_AllocateMemory(
            d->context.device, 
            size, 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            &UBO_memory[i], 
            &allocator
        );
        VK_CHECK(result, "failed to allocate UBO memory!");
    }

    // Create UBO set layout and layout binding
    UBO_binding.binding         = binding;
    UBO_binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    UBO_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UBO_binding.descriptorCount = 1;

    UBO_layoutCreateInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    UBO_layoutCreateInfo.bindingCount   = 1;
    UBO_layoutCreateInfo.pBindings      = &UBO_binding;
    result = vkCreateDescriptorSetLayout(
        d->context.device, 
        &UBO_layoutCreateInfo, 
        NULL, 
        &UBO_layout
    );
    VK_CHECK(result, "failed to create descriptor set layout!");

    // Create UBO descriptor sets
    UBO_setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    UBO_setAllocInfo.descriptorPool     = descriptorPool;
    UBO_setAllocInfo.descriptorSetCount = 1;
    UBO_setAllocInfo.pSetLayouts        = &UBO_layout;
    result = vkAllocateDescriptorSets(
        d->context.device, 
        &UBO_setAllocInfo, 
        &UBO_set
    );
    VK_CHECK(result, "failed to allocate UBO descriptor sets");

    return result;
}

void update_UBO(VkDevice device,
                const void *pData, 
                size_t size, 
                VkDescriptorSet set,
                uint32_t binding,
                ScVkMemoryBlock buffer)
{
    VkWriteDescriptorSet    setWrite    = {};
    VkCopyDescriptorSet     setCopies   = {};

    VkDescriptorBufferInfo  bufInfo     = {};

    bufInfo.buffer  = buffer.m_buffer;
    bufInfo.range   = VK_WHOLE_SIZE;
    bufInfo.offset  = 0;

    setWrite.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    setWrite.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    setWrite.descriptorCount    = 1;
    setWrite.dstSet             = set;
    setWrite.dstBinding         = binding;
    setWrite.pBufferInfo        = &bufInfo;

    vk_UploadToDeviceMemory(pData, size, buffer, &allocator);
    vkUpdateDescriptorSets(
        device, 
        1, 
        &setWrite, 
        0, 
        NULL
    );
}

void destroy_UBO(vkDisplay *d)
{
    vkDestroyDescriptorSetLayout(d->context.device, UBO_layout, NULL);
    vk_FreeMemory(UBO_memory[0], &allocator);
    vk_FreeMemory(UBO_memory[1], &allocator);
    vkDestroyDescriptorPool(d->context.device, descriptorPool, NULL);
}
#pragma endregion
void record_commands(vkDisplay *d)
{
    VkClearValue                clear = {1.0f, 0.0f, 0.0f, 1.0f};
 
    VkCommandBufferBeginInfo    cmdBeginInfo    = {};
    VkRenderPassBeginInfo       renderBeginInfo = {};
    
    // Begin command buffer
    cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkResetCommandBuffer(cmdBuffer, 0);
    vkBeginCommandBuffer(cmdBuffer, &cmdBeginInfo);

    // Begin render pass
    renderBeginInfo.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderBeginInfo.renderPass          = d->renderState.renderPass;
    renderBeginInfo.renderArea.offset   = (VkOffset2D){0, 0};
    renderBeginInfo.renderArea.extent   = d->context.displayExtent;
    renderBeginInfo.clearValueCount     = 1;
    renderBeginInfo.pClearValues        = &clear;
    renderBeginInfo.framebuffer         = d->renderState.framebuffers[imageIndex];
    vkCmdBeginRenderPass(cmdBuffer, &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdPushConstants(
        cmdBuffer, 
        d->renderState.layouts[0], 
        VK_SHADER_STAGE_FRAGMENT_BIT, 
        0, 
        sizeof(float), 
        &time
    );

    vkCmdBindDescriptorSets(
        cmdBuffer, 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        d->renderState.layouts[0], 
        0, 
        1, 
        &UBO_set, 
        0, 
        NULL
    );

    vkCmdSetViewport(cmdBuffer, 0, 1, &d->renderState.viewport);
    vkCmdSetScissor(cmdBuffer, 0, 1, &d->renderState.scissor);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, d->renderState.pipelines[0]);
    
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vtxBlock.m_buffer, offsets);
    vkCmdBindIndexBuffer(cmdBuffer, idxBlock.m_buffer, 0, VK_INDEX_TYPE_UINT32);
    
    //vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    vkCmdDrawIndexed(cmdBuffer, iSize / sizeof(uint32_t), 1, 0, 0, 0);

    // End render pass and command buffer
    vkCmdEndRenderPass(cmdBuffer);
    vkEndCommandBuffer(cmdBuffer);
}

void start(vkDisplay *d)
{
    VkPushConstantRange range = {};
    VkResult result = VK_ERROR_UNKNOWN;

    d->renderState.layouts      = malloc(1*sizeof(VkPipelineLayout));
    d->renderState.pipelines    = malloc(1*sizeof(VkPipeline));
    d->renderState.size         = 1;

    // Create push constants and default pipeline
    range.stageFlags    = VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset        = 0;
    range.size          = sizeof(float);

    // Create UBO descriptor sets

    // Create descriptor pool
    result = init_descriptor_pool(d);
    VK_CHECK_VOID(result, "failed to create descriptor pool!");

    reserve_memory(d);
    result = create_UBO(d, 0, 0, 64);
    VK_CHECK_VOID(result, "failed to create UBO descriptor sets!");

    for (int i = 0; i < frameCount; i++)
    {
        update_UBO(
            d->context.device, 
            &modelMatrix, 
            sizeof(modelMatrix),
            UBO_set,
            0,
            UBO_memory[0]
        );
    }

    result = vk_CreateDefaultGraphicsPipeline(
        &d->context, 
        &d->renderState, 
        0, 
        &range, 1, 
        &UBO_layout, 1
    );
    VK_CHECK_VOID(result, "failed to create default graphics pipeline");

    result = vk_CreateCommandPool(&d->context, &d->renderState, &cmdPool);
    VK_CHECK_VOID(result, "failed to create command pool!");
    result = vk_AllocateCommandBuffers(&d->context, cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, &cmdBuffer);
    VK_CHECK_VOID(result, "failed to create command buffers!");

    VkSemaphoreCreateInfo   semaphoreInfo = {};
    VkFenceCreateInfo       fenceInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    fenceInfo.sType     = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    vkCreateSemaphore(d->context.device, &semaphoreInfo, NULL, &acquireImage);
    vkCreateSemaphore(d->context.device, &semaphoreInfo, NULL, &renderFinished);
    
    vkCreateFence(d->context.device, &fenceInfo, NULL, &frameInFlight);


}

void render(vkDisplay *d)
{
    VkSubmitInfo                submitInfo      = {};
    VkPresentInfoKHR            presentInfo     = {};

    VkPipelineStageFlags        flags[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkResult result = VK_ERROR_UNKNOWN;

    // Get next swapchain image
    vkAcquireNextImageKHR(d->context.device, d->renderState.swapchain, UINT32_MAX, acquireImage, VK_NULL_HANDLE, &imageIndex);

    // Record commands
    record_commands(d);

    // Submit command queue
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &cmdBuffer;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &acquireImage;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &renderFinished;
    submitInfo.pWaitDstStageMask    = flags;
    vkQueueSubmit(d->renderState.queue, 1, &submitInfo, frameInFlight);

    // Present image
    presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount  = 1;
    presentInfo.pWaitSemaphores     = &renderFinished;
    presentInfo.swapchainCount      = 1;
    presentInfo.pSwapchains         = &d->renderState.swapchain;
    presentInfo.pImageIndices       = &imageIndex;
    presentInfo.pResults            = &result;
    vkQueuePresentKHR(d->renderState.queue, &presentInfo);

    // Reset fences
    vkWaitForFences(d->context.device, 1, &frameInFlight, VK_TRUE, UINT32_MAX);
    vkResetFences(d->context.device, 1, &frameInFlight);
}

void cleanup(vkDisplay *d)
{
    destroy_UBO(d);
    clean_memory(d);

    vkDestroyPipeline(d->context.device, d->renderState.pipelines[0], NULL);
    vkDestroyPipelineLayout(d->context.device, d->renderState.layouts[0], NULL);

    free(d->renderState.layouts);
    free(d->renderState.pipelines);

    vkDeviceWaitIdle(d->context.device);

    vk_FreeCommandBuffers(&d->context, cmdPool, 1, &cmdBuffer);
    vk_DestroyCommandPool(&d->context, cmdPool);

    vkDestroySemaphore(d->context.device, acquireImage, NULL);
    vkDestroySemaphore(d->context.device, renderFinished, NULL);
    
    vkDestroyFence(d->context.device, frameInFlight, NULL);
}

#define PI 3.14159265359
void update(vkDisplay *d)
{
    time = SDL_GetTicks();
    modelMatrix[12] = (sin(time / 10.f * PI / 180.f));
    modelMatrix[13] = (sin(time / 10.f * PI / 180.f));

    update_UBO(
        d->context.device, 
        &modelMatrix, 
        sizeof(modelMatrix), 
        UBO_set, 
        0, 
        UBO_memory[imageIndex]
    );
}

#include "vk_memory.h"

int main()
{   
    int err_code = 0,
        exit = 0;
    vkDisplay disp;
    SDL_Event e;

    err_code = vk_new_display(800, 600, "test", &disp);
    //reserve_memory(&disp);
    //vk_PrintPhysicalDeviceMemoryProperties(disp.renderState.activeDevice);

    start(&disp);
        
    for (;;)
    {
        SDL_PollEvent(&e);
        switch (e.type)
        {
            case SDL_QUIT:
                exit = 1;
                break;
        }

        update(&disp);
        render(&disp);

        if (exit != 0)
            break;
    }
    cleanup(&disp);
    vk_destroy_display(&disp);
    return 0;
}