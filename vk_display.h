#pragma once

#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL.h>
#include <vulkan/vulkan_core.h>

#include "vk_common.h"
#include "vk_error.h"
#include "vk_init.h"

// not min and max to avoid naming conflict with math min and max
#define smallest(x, y) ((x < y) ? x : y)
#define biggest(x, y) ((x > y) ? x : y)

#pragma region [ Vulkan Window Defs ]
typedef struct
{
    int     w, h;
    char    title[128];
    float   ar;

    SDL_Renderer    *ren;
    SDL_Window      *win;

    vkContext       context;
    vkRenderState   renderState;
} vkDisplay;
#pragma endregion
#pragma region [ Vulkan Window Interface ]
static int vk_new_display(int w, int h, const char *title, vkDisplay* d)
{
    size_t  title_len;
    int     err_code = 0;

    // Try to init SDL first
    if ((err_code = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) != 0)
        goto sdl_init_err;

    // Set window title
    title_len = strlen(title);
    memcpy(d->title, title, smallest(128, title_len));

    // Set width, height, aspect ratio
    d->w = w;
    d->h = h;
    d->ar = (float)w / (float)h;

    // Create the window object
    d->win = SDL_CreateWindow
    (
        title, 
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        w,
        h,
        SDL_WINDOW_VULKAN
    );
    if (d->win == NULL)
        goto win_err;

    // Init vulkan, create the instance and surface
    err_code = vk_CreateDefaultInstanceAndSurface(&d->context, d->win);
    if (err_code != VK_SUCCESS)
        goto vk_init_err;
    
    err_code = vk_GetLogicalDevice(&d->context);
    if (err_code != VK_SUCCESS)
        goto vk_dev_err;

    d->renderState.deviceIndex              = d->context.db.deviceIndex;
    d->renderState.gfxQueueFamilyIndex      = d->context.db.queueIndex;
    d->renderState.activeDevice             = d->context.db.activeDevice;
    d->renderState.activeDevProperties      = d->context.db.devProperties[d->context.db.deviceIndex];
    d->renderState.activeFmt                = d->context.db.surfaceFormats[d->context.db.deviceIndex][0];

    vk_CreateDefaultSwapchain(&d->context, &d->renderState, w, h);
    vk_CreateImageViews(&d->context, &d->renderState);
    vk_CreateRenderPass(&d->context, &d->renderState);
    vk_CreateFramebuffers(&d->context, &d->renderState);

    return err_code;
    sdl_init_err:
        fprintf(stderr, "error: %s\n", SDL_GetError());
        return err_code = 1;
    win_err:
        fprintf(stderr, "error: %s\n", SDL_GetError());
        return 2;
    vk_init_err:
        return err_code;
    vk_dev_err:
        fprintf(stderr, "error: failed to create vulkan device! (%s)\n", VkResultToCString((VkResult)err_code));
        return err_code;
}

static void vk_destroy_display(vkDisplay *d)
{
    vk_DestroyRenderState(&d->context, &d->renderState);
    vk_DestroyVulkan(&d->context);

    if (d->win != NULL)
        SDL_DestroyWindow(d->win);
        
    d->w = -1;
    d->h = -1;
    d->ar = 0.0f;

    SDL_Quit();
}
#pragma endregion