#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>

// Taken from Greg Hewgill, https://stackoverflow.com/a/600306
#define isPowerOfTwo(x) ((x != 0) && ((x & (x - 1)) == 0))
#pragma region [ Common Definitions ]
typedef size_t ScMemorySize;
typedef enum 
{
    SC_SUCCESS,
    SC_MEMORY_ALLOCATION_ERROR,
    SC_OUT_OF_MEMORY_ERROR,
    SC_ALIGNMENT_ERROR
}
ScResult;
#pragma endregion
#pragma region [ CList Macros ]
#define CList_init(T)                      CList_init_call(sizeof(T))
#define CList_reserve(T, cap)              CList_init_reserve(sizeof(T), cap)
#define CList_resize(list, cap)            CList_resize_call(&list, cap)
#define CList_push_back(list, data)        CList_push_back_call(&list, data)
#define CList_pop_back(list)               CList_pop_back_call(&list)
#define CList_destroy(list)                CList_destroy_call(&list)
#define CList_at(T, list, index)           ((T*)list.data)[index]
#pragma endregion
#pragma region [ CList Defs ]
typedef struct CList CList;
static CList CList_init_reserve(size_t stride, size_t capacity);
static void CList_reserve_call(CList *list, size_t capacity);
static void CList_resize_call(CList *list, size_t new_capacity);
static void CList_push_back_call(CList *list, const void *data);
static void CList_pop_back_call(CList *list);
static void CList_destroy_call(CList *list);
static void* CList_at_call(CList *list, size_t index);

typedef struct CList
{
    void    *data;
    size_t  stride,
            size,
            capacity;

    void*   (*at)(CList*, size_t);
    void    (*reserve)(CList*, size_t);
    void    (*resize)(CList*, size_t);
    void    (*push_back)(CList*, const void*);
    void    (*pop_back)(CList*);
    void    (*insert)(CList*, size_t, const void*);
    void    (*erase)(CList*, size_t);
    void    (*destroy)(CList*);
} CList;
#pragma endregion
#pragma region [ CList Interface ]
static CList CList_init_call(size_t stride)
{
    CList list = {};

    list.stride     = stride;
    list.at         = CList_at_call;
    list.reserve    = CList_reserve_call;
    list.push_back  = CList_push_back_call;
    list.pop_back   = CList_pop_back_call;
    list.destroy    = CList_destroy_call;

    return list;
}

static void* CList_at_call(CList *list, size_t index)
{
    return (void*)(((uintptr_t)list->data)+index*list->stride);
}

static void CList_reserve_call(CList *list, size_t capacity)
{
    if (list->capacity >= capacity)
        return;
    
    if (list->data == NULL)
        list->data = malloc(capacity*list->stride);
    else
        list->data = realloc(list->data, capacity*list->stride);

    list->capacity = capacity;
}

static CList CList_init_reserve(size_t stride, size_t capacity)
{
    CList list = CList_init_call(stride);
    CList_reserve_call(&list, capacity);

    return list;
};

static void CList_resize_call(CList *list, size_t new_capacity)
{
    list->capacity  = new_capacity;
    list->data      = realloc(list->data, new_capacity*list->stride);
}

static void CList_push_back_call(CList *list, const void *data)
{
    void* data_ptr;
    if (list->size == list->capacity)
        CList_resize_call(list, list->capacity*2);

    data_ptr = (void*)((uintptr_t)list->data + (list->size++*list->stride));

    memcpy(data_ptr, data, list->stride);
}

static void CList_insert_call(CList *list, size_t index, void* data)
{
    
}

static void CList_erase_call(CList *list, size_t index)
{

}

static void CList_pop_back_call(CList *list)
{
    if (list->size > 0)
        list->size--;
}

static void CList_destroy_call(CList *list)
{
    if (list->data != NULL)
        free(list->data);
    
    list->stride    = 0;
    list->size      = 0;
}
#pragma endregion
