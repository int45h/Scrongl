#pragma once

#include "common.h"

#define SC_ALIGN_POWER_OF_TWO(x,offset) (((x) + offset) & (~(offset-1)))

#pragma region [ Arena Defs ]
// This doesn't address alignment at the moment
// As an arena allocator, there is no concept of individual "free"s,
// since the whole arena will be freed. This avoids memory fragmentation
// at the cost of flexibility
typedef size_t ScMemorySize;
typedef struct ScArena
{
    uintptr_t*  m_data;
    size_t      m_size,
                m_ptr;
    uint32_t    m_offset;
}
ScArena;
#pragma endregion
#pragma region [ Arena Interace ]
static ScResult scMemory_CreateArena(   ScMemorySize size,  // Size in bytes
                                        uint32_t alignment, // Alignment in bytes (MUST BE power of two)
                                        ScArena *pArena)    // Pointer to arena
{
    if (!isPowerOfTwo(alignment))
        return SC_ALIGNMENT_ERROR;

    pArena->m_size      = size;
    pArena->m_ptr       = 0;
    pArena->m_data      = (uintptr_t*)malloc(size);
    pArena->m_offset    = alignment;

    if (pArena->m_data == NULL)
        return SC_MEMORY_ALLOCATION_ERROR;
    
    return SC_SUCCESS;
}

static ScResult scMemory_ArenaAlloc(ScMemorySize size, ScArena *pArena, void **ppData)
{
    ScMemorySize aligned_ptr = SC_ALIGN_POWER_OF_TWO(
        pArena->m_ptr+size, 
        pArena->m_offset
    );

    if ((pArena->m_ptr + size) > pArena->m_size)
        return SC_OUT_OF_MEMORY_ERROR;

    *ppData = (void*)(pArena->m_data + pArena->m_ptr);
    pArena->m_ptr = aligned_ptr;
    
    return SC_SUCCESS;
}

static void scMemory_DestroyArena(ScArena *pArena)
{
    if (pArena->m_data != NULL)
        free(pArena->m_data);
}
#pragma endregion