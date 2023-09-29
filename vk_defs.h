#pragma once

#include <vulkan/vulkan_core.h>

#include "vk_error.h"
#include "common.h"

#define alloc(n, t) ((t*)malloc(n*sizeof(t)))
#define VK_CHECK(result, error) if (result != VK_SUCCESS) {\
    fprintf(stderr, "error: %s (%s)\n", error, VkResultToCString(result));\
    return result;\
}

#define VK_CHECK_VOID(result, error) if (result != VK_SUCCESS) {\
    fprintf(stderr, "error: %s (%s)\n", error, VkResultToCString(result));\
}

#pragma region [ Utility Structs ]
typedef struct
{
    VkPhysicalDevice            *devices;
    VkPhysicalDeviceProperties  *devProperties;
    VkQueueFamilyProperties     **queueFamilies;
    VkSurfaceFormatKHR          **surfaceFormats;
    VkPresentModeKHR            **presentFormats;

    uint32_t            deviceCount,
                        queueFamilyListCount;
    uint32_t            *deviceQueueFamilyCount,
                        *surfaceFmtCount,
                        *presentFmtCount;

    VkPhysicalDevice    activeDevice;

    uint32_t            deviceIndex,
                        queueIndex,
                        gfxQueueFamilyIndex,
                        presQueueFamilyIndex;
    
} vkPhysicalDeviceDB;

typedef struct
{
    char            **availableExtensions;
    unsigned int    availableExtCount;
} vkExtensions;
#pragma endregion
#pragma region [ Vulkan Context ]
typedef struct
{
    VkInstance          instance;           // Vk Instance
    VkSurfaceKHR        surface;            // Window surface
    VkDevice            device;             // Logical device
    
    vkPhysicalDeviceDB  db;
    VkExtent2D          displayExtent;
} vkContext;
#pragma endregion
#pragma region [ Vulkan Render State ]
typedef struct
{
    // Shader modules
    VkShaderModule  vertexStage,
                    fragmentStage;

    // Swapchain and queue
    VkSwapchainKHR  swapchain;
    VkImage         *imageHandles;
    VkImageView     *imageViews;
    uint32_t        imageCount;
    VkQueue         queue;

    // Render pass and frame buffers
    VkRenderPass    renderPass;
    VkFramebuffer   *framebuffers;

    // Device
    VkPhysicalDevice            activeDevice;       // Active device
    VkSurfaceFormatKHR          activeFmt;          // Active format
    VkPhysicalDeviceProperties  activeDevProperties;// Active device properties

    // Device and queue indices
    uint32_t    deviceIndex,
                gfxQueueFamilyIndex,
                presQueueFamilyIndex;

    // Pipeline
    VkPipelineLayout    *layouts;
    VkPipeline          *pipelines;
    uint32_t            size;

    // Viewport and scissor
    VkViewport  viewport;
    VkRect2D    scissor;
} vkRenderState;
#pragma endregion
#pragma region [ Vulkan Device DB Interface ]
static void vk_BuildPhysicalDeviceDatabase(vkContext *context, vkPhysicalDeviceDB *db)
{
    // Variables
    VkPhysicalDevice    activeDevice;
    VkSurfaceKHR        activeSurface;
    int highestQueueCount = 0;

    // Get all physical devices
    vkEnumeratePhysicalDevices(context->instance, &db->deviceCount, NULL);
    db->devices = alloc(db->deviceCount, VkPhysicalDevice);
    vkEnumeratePhysicalDevices(context->instance, &db->deviceCount, db->devices);

    // Setup a list of device properties
    db->devProperties = alloc(db->deviceCount, VkPhysicalDeviceProperties);

    // Setup a list for queue families, and surface/present formats of each device
    db->queueFamilyListCount    = db->deviceCount;
    db->queueFamilies           = alloc(db->deviceCount, VkQueueFamilyProperties*);
    db->deviceQueueFamilyCount  = alloc(db->deviceCount, uint32_t);

    db->surfaceFormats          = alloc(db->deviceCount, VkSurfaceFormatKHR*);
    db->surfaceFmtCount         = alloc(db->deviceCount, uint32_t);
    db->presentFormats          = alloc(db->deviceCount, VkPresentModeKHR*);
    db->presentFmtCount         = alloc(db->deviceCount, uint32_t);

    db->deviceIndex = 0;
    db->queueIndex  = 0;

    // Get device properties and queue family properties of each device
    for (int dIndex = 0; dIndex < db->deviceCount; dIndex++)
    {
        // Set active device here so I don't have to keep fetching it
        activeDevice    = db->devices[dIndex];
        activeSurface   = context->surface;

        vkGetPhysicalDeviceProperties(activeDevice, &db->devProperties[dIndex]);

        // Allocate queue properties
        vkGetPhysicalDeviceQueueFamilyProperties(activeDevice, &db->deviceQueueFamilyCount[dIndex], NULL);
        db->queueFamilies[dIndex] = alloc(db->deviceQueueFamilyCount[dIndex], VkQueueFamilyProperties);
        vkGetPhysicalDeviceQueueFamilyProperties(activeDevice, &db->deviceQueueFamilyCount[dIndex], db->queueFamilies[dIndex]);

        // Allocate available surface formats
        vkGetPhysicalDeviceSurfaceFormatsKHR(activeDevice, activeSurface, &db->surfaceFmtCount[dIndex], NULL);
        db->surfaceFormats[dIndex] = alloc(db->surfaceFmtCount[dIndex], VkSurfaceFormatKHR);
        vkGetPhysicalDeviceSurfaceFormatsKHR(activeDevice, activeSurface, &db->surfaceFmtCount[dIndex], db->surfaceFormats[dIndex]);

        // Allocate available present formats
        vkGetPhysicalDeviceSurfacePresentModesKHR(activeDevice, activeSurface, &db->presentFmtCount[dIndex], NULL);
        db->presentFormats[dIndex] = alloc(db->presentFmtCount[dIndex], VkPresentModeKHR);
        vkGetPhysicalDeviceSurfacePresentModesKHR(activeDevice, activeSurface, &db->presentFmtCount[dIndex], db->presentFormats[dIndex]);

        // Rank each device
        for (int qIndex = 0; qIndex < db->deviceQueueFamilyCount[dIndex]; qIndex++)
        {
            // Get the queue flags 
            VkQueueFlags    flags = db->queueFamilies[dIndex][qIndex].queueFlags,
                            mask = (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
            
            // Select the device with the highest queue count and the most features
            if ((db->queueFamilies[dIndex][qIndex].queueCount > highestQueueCount) &&
                ((flags & mask) == mask))
            {
                db->deviceIndex = dIndex;
                db->queueIndex = qIndex;
                db->gfxQueueFamilyIndex = qIndex;

                VkBool32 supported = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(activeDevice, qIndex, context->surface, &supported);
                if (supported == VK_TRUE)
                    db->presQueueFamilyIndex = qIndex;

                highestQueueCount = db->queueFamilies[dIndex][qIndex].queueCount;
            }
        }
    }

    // Select the device
    db->activeDevice = db->devices[db->deviceIndex];
}

static void vk_DestroyPhysicalDeviceDatabase(vkPhysicalDeviceDB *db)
{
    if (db->devices != NULL)
        free(db->devices);
    if (db->devProperties != NULL)
        free(db->devProperties);
    
    if (db->queueFamilies != NULL)
    {
        while((db->queueFamilyListCount--) > 0)
        {
            if (db->queueFamilies[db->queueFamilyListCount] != NULL)
                free(db->queueFamilies[db->queueFamilyListCount]);
            if (db->surfaceFormats[db->queueFamilyListCount] != NULL)
                free(db->surfaceFormats[db->queueFamilyListCount]);
            if (db->presentFormats[db->queueFamilyListCount] != NULL)
                free(db->presentFormats[db->queueFamilyListCount]);
        }
    }
    if (db->deviceQueueFamilyCount != NULL)
        free(db->deviceQueueFamilyCount);
    if (db->surfaceFmtCount != NULL)
        free(db->surfaceFmtCount);
    if (db->presentFmtCount != NULL)
        free(db->presentFmtCount);
}
#pragma endregion