#pragma once
#include <math.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

#include "vk_allocator.h"
#include "vk_defs.h"

#pragma region [ Misc. Utilities ]
// Utilities
static void clearCharArray(char *arr, uint32_t arr_len)
{
    memset(arr, 0, arr_len);
}

static void getFileSizeFormatted(   VkDeviceSize size, 
                                    char* str, 
                                    uint32_t str_max_len)
{
    const char *file_size_prefixes[] = {
        "B",
        "KiB",
        "MiB",
        "GiB",
        "TiB"
    };
    int     index = (int)(log2((float)size) * 0.1f);
    float   size_reduced = (float)size / (float)((uint64_t)1<<(index*10));

    snprintf(str, str_max_len, "%.2f %s", 
        size_reduced, 
        file_size_prefixes[index]);

}

// WARNING: THIS FUNCTION IS VERRRRRRRY DANGEROUS!!!! 
// DO NOT USE THIS FUNCTION IN PRODUCTION UNTIL IT IS MADE SAFER
static void vk_GetMemoryTypeFlagsList(  VkMemoryPropertyFlags flags, 
                                        char* buf, 
                                        uint32_t buf_len,
                                        uint32_t tab_level)
{
    uint32_t ptr = 0;

    if((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) > 0)         {for (int i=0;i<tab_level;i++)buf[ptr++]='\t'; ptr+=snprintf(buf+ptr, buf_len-ptr,"DEVICE_LOCAL_BIT\n");}
    if((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) > 0)         {for (int i=0;i<tab_level;i++)buf[ptr++]='\t'; ptr+=snprintf(buf+ptr, buf_len-ptr,"HOST_VISIBLE_BIT\n");}
    if((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) > 0)        {for (int i=0;i<tab_level;i++)buf[ptr++]='\t'; ptr+=snprintf(buf+ptr, buf_len-ptr,"HOST_COHERENT_BIT\n");}
    if((flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) > 0)          {for (int i=0;i<tab_level;i++)buf[ptr++]='\t'; ptr+=snprintf(buf+ptr, buf_len-ptr,"HOST_CACHED_BIT\n");}
    if((flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) > 0)     {for (int i=0;i<tab_level;i++)buf[ptr++]='\t'; ptr+=snprintf(buf+ptr, buf_len-ptr,"LAZILY_ALLOCATED_BIT\n");}
    if((flags & VK_MEMORY_PROPERTY_PROTECTED_BIT) > 0)            {for (int i=0;i<tab_level;i++)buf[ptr++]='\t'; ptr+=snprintf(buf+ptr, buf_len-ptr,"PROTECTED_BIT\n");}
    if((flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) > 0)  {for (int i=0;i<tab_level;i++)buf[ptr++]='\t'; ptr+=snprintf(buf+ptr, buf_len-ptr,"DEVICE_COHERENT_BIT_AMD\n");}
    if((flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) > 0)  {for (int i=0;i<tab_level;i++)buf[ptr++]='\t'; ptr+=snprintf(buf+ptr, buf_len-ptr,"DEVICE_UNCACHED_BIT_AMD\n");}
    if((flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV) > 0)      {for (int i=0;i<tab_level;i++)buf[ptr++]='\t'; ptr+=snprintf(buf+ptr, buf_len-ptr,"RDMA_CAPABLE_BIT_NV\n");}
}

static void vk_PrintPhysicalDeviceMemoryProperties(VkPhysicalDevice device)
{
    VkPhysicalDeviceMemoryProperties devMemProperties = {};
    vkGetPhysicalDeviceMemoryProperties(device, &devMemProperties);

    char    file_size_str[128],
            memory_type_flags[1024];
    printf("\n\n");
    // List all heap types
    for (int heap = 0; heap < devMemProperties.memoryHeapCount; heap++)
    {
        clearCharArray(file_size_str, sizeof(file_size_str)/sizeof(char));
        getFileSizeFormatted(
            devMemProperties.memoryHeaps[heap].size, 
            file_size_str, 
            sizeof(file_size_str)/sizeof(char)
        );

        printf("Heap[%d]:\n\tSize (bytes): %lu (%s)\n\tFlags: %08X\n",
            heap,
            devMemProperties.memoryHeaps[heap].size,
            file_size_str,
            devMemProperties.memoryHeaps[heap].flags
        );

        for (int type = 0; type < devMemProperties.memoryTypeCount; type++)
        {
            if (devMemProperties.memoryTypes[type].heapIndex != heap)
                continue;

            clearCharArray(memory_type_flags, sizeof(memory_type_flags)/sizeof(char));
            vk_GetMemoryTypeFlagsList(
                    devMemProperties.memoryTypes[type].propertyFlags, 
                    memory_type_flags, 
                    sizeof(memory_type_flags)/sizeof(char), 
                    3
            );

            printf("\tMemoryTypes[%d]:\n\t\tHeap Index: %d\n\t\tFlags: \n%s\n",
                type,
                devMemProperties.memoryTypes[type].heapIndex,
                memory_type_flags
            );
        }
    }
}

static void vk_GetPhysicalDeviceType(   VkPhysicalDevice device, 
                                        VkPhysicalDeviceType *pDevType)
{
    VkPhysicalDeviceProperties devProperties = {};
    vkGetPhysicalDeviceProperties(device, &devProperties);

    *pDevType = devProperties.deviceType;
}

// Find memory types and get the memory indices for this
static void vk_FindMemoryType(  VkPhysicalDevice activeDevice, 
                                uint32_t featureMask,
                                uint32_t *pMemoryHeapIndex, 
                                uint32_t *pMemoryTypeIndex)
{
    VkPhysicalDeviceMemoryProperties memInfo = {};

    *pMemoryHeapIndex = -1;
    *pMemoryTypeIndex = -1;
    
    // Get device properties
    vkGetPhysicalDeviceMemoryProperties(activeDevice, &memInfo);

    // Find the memory type with the given feature mask
    for (int mType = 0; mType < memInfo.memoryTypeCount; mType++)
    {
        if ((memInfo.memoryTypes[mType].propertyFlags & featureMask) == 0)
            continue;

        *pMemoryHeapIndex = memInfo.memoryTypes[mType].heapIndex;
        *pMemoryTypeIndex = mType;

        return;
    }

    fprintf(stderr, "error: failed to find memory type with HOST_VISIBLE and HOST_COHERENT bits set!\n");
}
#pragma endregion
#pragma region [ Vulkan Global Allocator Defs ]
typedef struct 
{
    VkBuffer        m_buffer;
    VkDeviceSize    m_offset,
                    m_size;
}
ScVkMemoryBlock;

typedef struct
{
    // Staging buffer stuff
    VkDeviceMemory  m_memoryHandle;
    VkDeviceSize    m_size;
    VkBuffer        m_buffer;

    uint32_t        m_memoryTypeIndex,
                    m_memoryHeapIndex;

    VkCommandPool   m_cmdPool;
    VkCommandBuffer m_cmdBuffer;
}
ScVkStagingBuffer;

typedef struct
{
    VkDeviceMemory  m_memoryHandle;
    VkDevice        m_device;

    VkDeviceSize    m_dataOffset,
                    m_capacity,
                    m_alignment;

    uint32_t        m_memoryTypeIndex,
                    m_memoryHeapIndex,
                    m_queueIndex;
    
    // Used to determine if a staging buffer is needed
    VkPhysicalDeviceType    m_devType;
    ScVkStagingBuffer       m_stagingBuffer;
}
ScVkAllocator;
#pragma endregion
#pragma region [ Vulkan Buffer ]
static VkResult vk_CreateBuffer(VkDevice device, 
                                uint32_t queueIndex, 
                                VkDeviceSize size, 
                                VkBufferUsageFlags usage, 
                                VkBuffer *pBuffer)
{
    VkBufferCreateInfo bufInfo = {};

    bufInfo.sType                   = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.queueFamilyIndexCount   = 1;
    bufInfo.pQueueFamilyIndices     = &queueIndex;
    bufInfo.sharingMode             = VK_SHARING_MODE_EXCLUSIVE;
    bufInfo.size                    = size;
    bufInfo.usage                   = usage;

    return vkCreateBuffer(device, &bufInfo, NULL, pBuffer);
}

static void vk_DestroyBuffer(VkDevice device, VkBuffer buffer)
{
    vkDestroyBuffer(device, buffer, NULL);
}
#pragma endregion
#pragma region [ Vulkan Staging Buffer]
#define SCVK_DEFAULT_STAGING_BUFFER_SIZE 268435456
static VkResult vk_InitStagingBuffer(   VkDevice device, 
                                        vkRenderState *state, 
                                        VkDeviceSize size,
                                        ScVkStagingBuffer *pBuffer)
{
    VkMemoryRequirements        requirements = {};
    VkMemoryAllocateInfo        allocInfo = {};
    VkCommandPoolCreateInfo     cmdPoolInfo = {};
    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    VkResult result = VK_ERROR_UNKNOWN;

    // Allocate memory for the staging buffer (256 MiB)
    vk_FindMemoryType(  
        state->activeDevice, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        &pBuffer->m_memoryHeapIndex, 
        &pBuffer->m_memoryTypeIndex
    );

    // Create the staging buffer object
    result = vk_CreateBuffer(
        device, 
        state->gfxQueueFamilyIndex, 
        size, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
        &pBuffer->m_buffer
    );
    VK_CHECK(result, "failed to create staging buffer");
    
    // Get requirements
    vkGetBufferMemoryRequirements(
        device, 
        pBuffer->m_buffer, 
        &requirements
    );
    pBuffer->m_size = size;

    // Allocate memory for staging buffer
    allocInfo.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.memoryTypeIndex   = pBuffer->m_memoryTypeIndex;
    allocInfo.allocationSize    = requirements.size;
    result = vkAllocateMemory(device, &allocInfo, NULL, &pBuffer->m_memoryHandle);
    VK_CHECK(result, "failed to allocate memory");

    // Bind the buffer handle to the actual memory of the staging buffer
    result = vkBindBufferMemory(
        device, 
        pBuffer->m_buffer, 
        pBuffer->m_memoryHandle, 
        0
    );
    VK_CHECK(result, "failed to bind to staging buffer memory");

    // Create command pool and buffers
    cmdPoolInfo.sType               = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.flags               = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolInfo.queueFamilyIndex    = state->gfxQueueFamilyIndex;
    result = vkCreateCommandPool(
        device, 
        &cmdPoolInfo, 
        NULL, 
        &pBuffer->m_cmdPool
    );
    VK_CHECK(result, "failed to create staging buffer command pool");

    cmdAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool        = pBuffer->m_cmdPool;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    result = vkAllocateCommandBuffers(device, &cmdAllocInfo, &pBuffer->m_cmdBuffer);
    VK_CHECK(result, "failed to allocate staging buffer command buffers");

    return result;
}

static VkResult vk_TransferFromStagingBuffer(   vkContext *context,
                                                vkRenderState *state,
                                                ScVkMemoryBlock *pBlock,
                                                ScVkAllocator *pAllocator,
                                                VkDeviceSize blockOffset)
{
    VkCommandBufferBeginInfo beginInfo = {};
    VkSubmitInfo submitInfo = {};
    VkBufferCopy regionInfo = {};
    VkResult result = VK_ERROR_UNKNOWN;

    // Specify copy regions
    regionInfo.size = (pBlock->m_size < pAllocator->m_stagingBuffer.m_size)
        ? pBlock->m_size
        : pAllocator->m_stagingBuffer.m_size;
    regionInfo.srcOffset = 0;
    regionInfo.dstOffset = blockOffset;

    // Begin the transfer
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkResetCommandBuffer(pAllocator->m_stagingBuffer.m_cmdBuffer, 0);
    vkBeginCommandBuffer(pAllocator->m_stagingBuffer.m_cmdBuffer, &beginInfo);
    vkCmdCopyBuffer(
        pAllocator->m_stagingBuffer.m_cmdBuffer,
        pAllocator->m_stagingBuffer.m_buffer, 
        pBlock->m_buffer, 
        1, 
        &regionInfo
    );
    vkEndCommandBuffer(pAllocator->m_stagingBuffer.m_cmdBuffer);

    // Submit to device queue
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    result = vkQueueSubmit(state->queue, 1, &submitInfo, VK_NULL_HANDLE);
    VK_CHECK(result, "failed to submit to queue");
    
    return vkDeviceWaitIdle(context->device);
}

static void vk_DestroyStagingBuffer(    VkDevice device,
                                        ScVkStagingBuffer *pBuffer)
{
    vkDestroyBuffer(device, pBuffer->m_buffer, NULL);
    vkFreeMemory(device, pBuffer->m_memoryHandle, NULL);
}
#pragma endregion
#pragma region [ Vulkan Global Allocator Init ]
static VkResult vk_AllocateGlobalMemory(    VkDevice device,
                                            ScVkAllocator *pAllocator)
{
    VkMemoryAllocateInfo allocInfo = {};
    VkResult result = VK_ERROR_UNKNOWN;

    allocInfo.sType             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize    = pAllocator->m_capacity;
    allocInfo.memoryTypeIndex   = pAllocator->m_memoryTypeIndex;

    return vkAllocateMemory(device, &allocInfo, NULL, &pAllocator->m_memoryHandle);
}

// Allocate memory with a staging buffer
static VkResult vk_InitGlobalAllocatorDiscrete( VkDevice device, 
                                                vkRenderState *state, 
                                                VkDeviceSize size,
                                                ScVkAllocator *pAllocator)
{
    VkMemoryRequirements requirements = {};

    VkBuffer    tmp,
                tmpStaging;
    VkResult    result = VK_ERROR_UNKNOWN;

    // Create staging buffer
    vk_InitStagingBuffer(
        device, 
        state, 
        SCVK_DEFAULT_STAGING_BUFFER_SIZE, 
        &pAllocator->m_stagingBuffer
    );

    // Allocate device local memory
    vk_FindMemoryType(
        state->activeDevice, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        &pAllocator->m_memoryHeapIndex, 
        &pAllocator->m_memoryTypeIndex
    );

    // Create a temporary buffer to get memory requirements
    result = vk_CreateBuffer(
        device, 
        state->gfxQueueFamilyIndex, 
        size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
        &tmp
    );
    VK_CHECK(result, "failed to create temporary buffer");

    // Get requirements
    vkGetBufferMemoryRequirements(device, tmp, &requirements);
    
    // Destroy temporary buffer
    vkDestroyBuffer(device, tmp, NULL);
    
    // Set allocator data here
    pAllocator->m_dataOffset        = 0;
    pAllocator->m_capacity          = requirements.size;
    pAllocator->m_alignment         = requirements.alignment;
    pAllocator->m_queueIndex        = state->gfxQueueFamilyIndex;

    // Allocate memory
    vk_AllocateGlobalMemory(device, pAllocator);
    return result;
}

// Allocate memory without using a staging buffer (technically not necessary since device memory is just CPU memory anyway)
static VkResult vk_InitGlobalAllocatorIntegrated(   VkDevice device, 
                                                    vkRenderState *state, 
                                                    VkDeviceSize size,
                                                    ScVkAllocator *pAllocator)
{
    VkMemoryRequirements requirements = {};

    VkBuffer tmp;
    VkResult result = VK_ERROR_UNKNOWN;

    uint32_t featureMask =  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // Set the staging buffer to NULL for now
    memset((void*)&pAllocator->m_stagingBuffer, 0, sizeof(ScVkStagingBuffer));

    // Find the appropriate memory type
    vk_FindMemoryType(  state->activeDevice, 
                        featureMask, 
                        &pAllocator->m_memoryHeapIndex, 
                        &pAllocator->m_memoryTypeIndex);

    // Create a temporary buffer to get memory requirements
    result = vk_CreateBuffer(
        device, 
        state->gfxQueueFamilyIndex, 
        size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
        &tmp
    );
    VK_CHECK(result, "failed to create buffer");

    // Get requirements
    vkGetBufferMemoryRequirements(device, tmp, &requirements);
    
    // Destroy temporary buffer
    vkDestroyBuffer(device, tmp, NULL);
    
    // Set allocator data here
    pAllocator->m_device            = device;
    pAllocator->m_dataOffset        = 0;
    pAllocator->m_capacity          = requirements.size;
    pAllocator->m_alignment         = requirements.alignment;
    pAllocator->m_queueIndex        = state->gfxQueueFamilyIndex;

    // Allocate memory
    return vk_AllocateGlobalMemory(device, pAllocator);
}

static VkResult vk_InitGlobalAllocator( VkDevice device, 
                                        vkRenderState *state, 
                                        VkDeviceSize size,
                                        ScVkAllocator *pAllocator)
{
    return vk_InitGlobalAllocatorIntegrated(device, state, size, pAllocator);

    //pAllocator->m_devType = state->activeDevProperties.deviceType;
//
    //switch (pAllocator->m_devType)
    //{
    //    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    //        return vk_InitGlobalAllocatorDiscrete(context, state, size, pAllocator);
    //    default:
    //        return vk_InitGlobalAllocatorIntegrated(context, state, size, pAllocator);
    //}
}

static void vk_DestroyGlobalAllocator(  vkContext *context, 
                                        ScVkAllocator *pAllocator)
{
    vkFreeMemory(context->device, pAllocator->m_memoryHandle, NULL);
}
#pragma endregion
#pragma region [ Vulkan Global Allocator Interace ]
static VkResult vk_AllocateMemory(  VkDevice device,
                                    VkDeviceSize size,
                                    VkBufferUsageFlags usage,
                                    ScVkMemoryBlock *pBlock, 
                                    ScVkAllocator *pAllocator)
{
    VkBufferCreateInfo bufInfo = {};
    VkResult result = VK_ERROR_UNKNOWN;
    
    // Setup block parameters
    pBlock->m_size      = SC_ALIGN_POWER_OF_TWO(size, pAllocator->m_alignment);
    pBlock->m_offset    = pAllocator->m_dataOffset;

    // For now, this allocator does not support global memory resizing, 
    // so fail if we run out of space
    if ((pAllocator->m_dataOffset + pBlock->m_size) > pAllocator->m_capacity)
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;

    // Create buffer
    result = vk_CreateBuffer(
        device, 
        pAllocator->m_queueIndex, 
        size, 
        usage, 
        &pBlock->m_buffer
    );
    VK_CHECK(result, "failed to create buffer");

    // Bind it to our device memory using the data pointer as an offset
    result = vkBindBufferMemory(
        device, 
        pBlock->m_buffer, 
        pAllocator->m_memoryHandle, 
        pAllocator->m_dataOffset
    );
    VK_CHECK(result, "failed to bind buffer to memory");

    // Update the data offset
    pAllocator->m_dataOffset += pBlock->m_size;
    return result;
}

// TO-DO: ideally, this would use a staging buffer, unless you are not using a discrete GPU.
static VkResult vk_UploadToDeviceMemory(const void *pData,
                                        size_t size,
                                        ScVkMemoryBlock block,
                                        ScVkAllocator *pAllocator)
{
    void *gpuMemory = NULL;
    VkResult result = VK_ERROR_UNKNOWN;

    // Don't bother if the size of the data exceeds the size of the block
    if (size > block.m_size)
        return VK_ERROR_UNKNOWN;

    // Map memory
    result = vkMapMemory(pAllocator->m_device, pAllocator->m_memoryHandle, block.m_offset, size, 0, &gpuMemory);
    VK_CHECK(result, "failed to map memory");
    
    // Copy here
    memcpy(gpuMemory, pData, size);
    vkUnmapMemory(pAllocator->m_device, pAllocator->m_memoryHandle);

    return result;
}



// Right now, this function is a no-op, meaning it does nothing.
// This is because the global allocator is little more than a
// linear allocator
static void vk_FreeMemory(  ScVkMemoryBlock block, 
                            ScVkAllocator *pAllocator)
{
    vkDestroyBuffer(pAllocator->m_device, block.m_buffer, NULL);
}
#pragma endregion