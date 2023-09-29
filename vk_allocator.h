#pragma once
#include "vk_defs.h"
#include "common.h"
#include <vulkan/vulkan_core.h>

#define SC_ALIGN_POWER_OF_TWO(x,offset) (((x) + offset) & (~(offset-1)))

#pragma region [ Vulkan Arena Defs ]
// This doesn't address alignment at the moment
// As an arena allocator, there is no concept of individual "free"s,
// since the whole arena will be freed. This avoids memory fragmentation
// at the cost of flexibility
typedef struct
{
    VkBuffer        m_data;
    VkDeviceMemory  m_devMemory;
    size_t          m_size,
                    m_ptr;
    uint32_t        m_offset;
}
ScVkBufferArena;
#pragma endregion
#pragma region [ Vulkan Arena Interface ]
static VkResult scVkMemory_CreateArena( VkDevice device,
                                        VkBufferUsageFlags usage,
                                        VkDeviceSize size, 
                                        uint32_t offset,
                                        VkBuffer buffer,
                                        VkDeviceMemory memory,
                                        ScVkBufferArena *pArena)
{
    VkResult result;
    result = vkBindBufferMemory(device, buffer, memory, offset);
    VK_CHECK(result, "failed to bind memory");

    pArena->m_devMemory = memory;
    pArena->m_size      = size;
    pArena->m_ptr       = 0;
    pArena->m_data      = buffer;
    
    return VK_SUCCESS;
}

static VkResult scVkMemory_ArenaAlloc(  VkDeviceSize size, 
                                        ScVkBufferArena *pArena, 
                                        VkDeviceSize *ptr)
{
    VkDeviceSize aligned_ptr = SC_ALIGN_POWER_OF_TWO(
        pArena->m_ptr+size, 
        pArena->m_offset
    );
    *ptr = pArena->m_ptr;

    if ((pArena->m_ptr + size) > pArena->m_size)
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;

    pArena->m_ptr = aligned_ptr;

    return VK_SUCCESS;
}

static VkResult scVkMemory_ArenaCopy(   VkDevice device, 
                                        ScVkBufferArena *pArena, 
                                        const void *pData, 
                                        VkDeviceSize size)
{
    void *GPU_mem;
    VkResult result = VK_ERROR_UNKNOWN;

    if ((pArena->m_ptr+size) > pArena->m_size)
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;

    result = vkMapMemory(device, pArena->m_devMemory, pArena->m_offset, size, 0, &GPU_mem);
    VK_CHECK(result, "failed to map memory!");

    memcpy(GPU_mem, pData, size);

    vkUnmapMemory(device, pArena->m_devMemory);
    return VK_SUCCESS;
}

static void scVkMemory_DestroyArena(    VkDevice device, 
                                        ScVkBufferArena *pArena, 
                                        const VkAllocationCallbacks *pAllocator)
{
    if (pArena->m_data != VK_NULL_HANDLE)
        vkDestroyBuffer(device, pArena->m_data, pAllocator);

    pArena->m_size  = SIZE_MAX;
    pArena->m_ptr   = SIZE_MAX;
}
#pragma endregion