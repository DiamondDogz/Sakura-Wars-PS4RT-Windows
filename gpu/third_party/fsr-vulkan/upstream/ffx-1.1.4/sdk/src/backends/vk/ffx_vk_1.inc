// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <mutex>
#include <FidelityFX/host/ffx_interface.h>
#include <FidelityFX/host/ffx_util.h>
#include <FidelityFX/host/ffx_assert.h>
#include <FidelityFX/host/backends/vk/ffx_vk.h>
#include <ffx_shader_blobs.h>
#include <ffx_breadcrumbs_list.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <codecvt>  // this is deprecated so it's just a fallback solution
#endif  // _WIN32

#include <vulkan/vulkan.h>

// prototypes for functions in the interface
FfxVersionNumber       GetSDKVersionVK(FfxInterface* backendInterface);
FfxErrorCode           GetEffectGpuMemoryUsageVK(FfxInterface* backendInterface, FfxUInt32 effectContextId, FfxEffectMemoryUsage* outVramUsage);
FfxErrorCode           CreateBackendContextVK(FfxInterface* backendInterface, FfxEffect effect, FfxEffectBindlessConfig* bindlessConfig, FfxUInt32* effectContextId);
FfxErrorCode           GetDeviceCapabilitiesVK(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities);
FfxErrorCode           DestroyBackendContextVK(FfxInterface* backendInterface, FfxUInt32 effectContextId);
FfxErrorCode           CreateResourceVK(FfxInterface* backendInterface, const FfxCreateResourceDescription* desc, FfxUInt32 effectContextId, FfxResourceInternal* outTexture);
FfxErrorCode           DestroyResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId);
FfxErrorCode           MapResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource, void** ptr);
FfxErrorCode           UnmapResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode           RegisterResourceVK(FfxInterface* backendInterface, const FfxResource* inResource, FfxUInt32 effectContextId, FfxResourceInternal* outResourceInternal);
FfxResource            GetResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode           UnregisterResourcesVK(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);
FfxErrorCode           RegisterStaticResourceVK(FfxInterface* backendInterface, const FfxStaticResourceDescription* desc, FfxUInt32 effectContextId);
FfxResourceDescription GetResourceDescriptionVK(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode           StageConstantBufferDataVK(FfxInterface* backendInterface, void* data, FfxUInt32 size, FfxConstantBuffer* constantBuffer);
FfxErrorCode           CreatePipelineVK(FfxInterface* backendInterface, FfxEffect effect, FfxPass passId, uint32_t permutationOptions, const FfxPipelineDescription* desc, FfxUInt32 effectContextId, FfxPipelineState* outPass);
FfxErrorCode           DestroyPipelineVK(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId);
FfxErrorCode           ScheduleGpuJobVK(FfxInterface* backendInterface, const FfxGpuJobDescription* job);
FfxErrorCode           ExecuteGpuJobsVK(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);
FfxErrorCode           BreadcrumbsAllocBlockVK(FfxInterface* backendInterface, uint64_t blockBytes, FfxBreadcrumbsBlockData* blockData);
void                   BreadcrumbsFreeBlockVK(FfxInterface* backendInterface, FfxBreadcrumbsBlockData* blockData);
void                   BreadcrumbsWriteVK(FfxInterface* backendInterface, FfxCommandList commandList, uint32_t value, uint64_t gpuLocation, void* gpuBuffer, bool isBegin);
void                   BreadcrumbsPrintDeviceInfoVK(FfxInterface* backendInterface, FfxAllocationCallbacks* allocs, bool extendedInfo, char** printBuffer, size_t* printSize);
void                   RegisterConstantBufferAllocatorVK(FfxInterface* backendInterface, FfxConstantBufferAllocator fpConstantAllocator);


static VkDeviceContext sVkDeviceContext = {};

#define MAX_PIPELINE_USAGE_PER_FRAME      (10) // Required to make sure passes that are called more than once per-frame don't have their descriptors overwritten.
#define MAX_DESCRIPTOR_SET_LAYOUTS        (64)
#define FFX_MAX_BINDLESS_DESCRIPTOR_COUNT (65536)

// Constant buffer allocation callback
static FfxConstantBufferAllocator s_fpConstantAllocator = nullptr;

// Offset the binding of samplers to avoid collisions
constexpr uint32_t SAMPLER_BINDING_SHIFT = 1000;

typedef struct BackendContext_VK {

    // store for resources and resourceViews
    typedef struct Resource
    {
#ifdef _DEBUG
        char                    resourceName[64] = {};
#endif
        union {
            VkImage             imageResource;
            VkBuffer            bufferResource;
        };

        FfxResourceDescription  resourceDescription;
        FfxResourceStates       initialState;
        FfxResourceStates       currentState;
        int32_t                 srvViewIndex;
        int32_t                 uavViewIndex;
        uint32_t                uavViewCount;

        VkDeviceMemory          deviceMemory;
        VkDeviceSize            allocationSize;
        VkMemoryPropertyFlags   memoryProperties;

        bool                    undefined;
        bool                    dynamic;

    } Resource;

    typedef struct PipelineLayout {

        VkSampler               samplers[FFX_MAX_SAMPLERS];
        VkDescriptorSetLayout   descriptorSetLayout;
        VkDescriptorSet         descriptorSets[FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME];
        uint32_t                descriptorSetIndex;
        VkPipelineLayout        pipelineLayout;
        int32_t                 staticTextureSrvSet;
        int32_t                 staticBufferSrvSet;
        int32_t                 staticTextureUavSet;
        int32_t                 staticBufferUavSet;
    } PipelineLayout;

    typedef struct VKFunctionTable
    {
        PFN_vkGetDeviceProcAddr                 vkGetDeviceProcAddr = 0;
        PFN_vkSetDebugUtilsObjectNameEXT        vkSetDebugUtilsObjectNameEXT = 0;
        PFN_vkCreateDescriptorPool              vkCreateDescriptorPool = 0;
        PFN_vkCreateSampler                     vkCreateSampler = 0;
        PFN_vkCreateDescriptorSetLayout         vkCreateDescriptorSetLayout = 0;
        PFN_vkCreateBuffer                      vkCreateBuffer = 0;
        PFN_vkCreateBufferView                  vkCreateBufferView = 0;
        PFN_vkCreateImage                       vkCreateImage = 0;
        PFN_vkCreateImageView                   vkCreateImageView = 0;
        PFN_vkCreateShaderModule                vkCreateShaderModule = 0;
        PFN_vkCreatePipelineLayout              vkCreatePipelineLayout = 0;
        PFN_vkCreateComputePipelines            vkCreateComputePipelines = 0;
        PFN_vkDestroyPipelineLayout             vkDestroyPipelineLayout = 0;
        PFN_vkDestroyPipeline                   vkDestroyPipeline = 0;
        PFN_vkDestroyImage                      vkDestroyImage = 0;
        PFN_vkDestroyImageView                  vkDestroyImageView = 0;
        PFN_vkDestroyBuffer                     vkDestroyBuffer = 0;
        PFN_vkDestroyBufferView                 vkDestroyBufferView = 0;
        PFN_vkDestroyDescriptorSetLayout        vkDestroyDescriptorSetLayout = 0;
        PFN_vkDestroyDescriptorPool             vkDestroyDescriptorPool = 0;
        PFN_vkDestroySampler                    vkDestroySampler = 0;
        PFN_vkDestroyShaderModule               vkDestroyShaderModule = 0;
        PFN_vkGetBufferMemoryRequirements       vkGetBufferMemoryRequirements = 0;
        PFN_vkGetBufferMemoryRequirements2      vkGetBufferMemoryRequirements2 = 0;
        PFN_vkGetImageMemoryRequirements        vkGetImageMemoryRequirements = 0;
        PFN_vkAllocateDescriptorSets            vkAllocateDescriptorSets = 0;
        PFN_vkFreeDescriptorSets                vkFreeDescriptorSets = 0;
        PFN_vkAllocateMemory                    vkAllocateMemory = 0;
        PFN_vkFreeMemory                        vkFreeMemory = 0;
        PFN_vkMapMemory                         vkMapMemory = 0;
        PFN_vkUnmapMemory                       vkUnmapMemory = 0;
        PFN_vkBindBufferMemory                  vkBindBufferMemory = 0;
        PFN_vkBindImageMemory                   vkBindImageMemory = 0;
        PFN_vkUpdateDescriptorSets              vkUpdateDescriptorSets = 0;
        PFN_vkFlushMappedMemoryRanges           vkFlushMappedMemoryRanges = 0;
        PFN_vkCmdPipelineBarrier                vkCmdPipelineBarrier = 0;
        PFN_vkCmdBindPipeline                   vkCmdBindPipeline = 0;
        PFN_vkCmdBindDescriptorSets             vkCmdBindDescriptorSets = 0;
        PFN_vkCmdDispatch                       vkCmdDispatch = 0;
        PFN_vkCmdDispatchIndirect               vkCmdDispatchIndirect = 0;
        PFN_vkCmdCopyBuffer                     vkCmdCopyBuffer = 0;
        PFN_vkCmdCopyImage                      vkCmdCopyImage = 0;
        PFN_vkCmdCopyBufferToImage              vkCmdCopyBufferToImage = 0;
        PFN_vkCmdClearColorImage                vkCmdClearColorImage = 0;
        PFN_vkCmdFillBuffer                     vkCmdFillBuffer = 0;
        PFN_vkCmdWriteBufferMarkerAMD           vkCmdWriteBufferMarkerAMD = 0;
        PFN_vkCmdWriteBufferMarker2AMD          vkCmdWriteBufferMarker2AMD = 0;
        PFN_vkCmdBeginDebugUtilsLabelEXT        vkCmdBeginDebugUtilsLabelEXT = 0;
        PFN_vkCmdEndDebugUtilsLabelEXT          vkCmdEndDebugUtilsLabelEXT = 0;

    } VkFunctionTable;

    uint32_t refCount;
    uint32_t maxEffectContexts;

    VkDevice                device = VK_NULL_HANDLE;
    VkPhysicalDevice        physicalDevice = VK_NULL_HANDLE;
    VkBool32                deviceCoherentMemoryEnabled = VK_FALSE;
    VkBool32                shaderFloat16Enabled = VK_FALSE;
    VkBool32                subgroupSizeControlEnabled = VK_FALSE;
    VkBool32                computeFullSubgroupsEnabled = VK_FALSE;
    VkBool32                synchronization2Enabled = VK_FALSE;
    VkBool32                bufferMarkerEnabled = VK_FALSE;
    VkBool32                debugUtilsEnabled = VK_FALSE;
    VkBool32                shaderStorageBufferArrayNonUniformIndexingEnabled = VK_FALSE;
    VkBool32                accelerationStructureEnabled = VK_FALSE;
    VkFunctionTable         vkFunctionTable = {};

    FfxGpuJobDescription*   pGpuJobs;
    uint32_t                gpuJobCount = 0;

    typedef struct VkResourceView {
        VkImageView imageView;
    } VkResourceView;
    VkResourceView*         pResourceViews;

    uint8_t*                pStagingRingBuffer;
    uint32_t                stagingRingBufferBase = 0;

    PipelineLayout*         pPipelineLayouts;

    VkDescriptorPool        descriptorPool;
    uint32_t                bindlessBase;

    VkImageMemoryBarrier    imageMemoryBarriers[FFX_MAX_BARRIERS] = {};
    VkBufferMemoryBarrier   bufferMemoryBarriers[FFX_MAX_BARRIERS] = {};
    uint32_t                scheduledImageBarrierCount = 0;
    uint32_t                scheduledBufferBarrierCount = 0;
    VkPipelineStageFlags    srcStageMask = 0;
    VkPipelineStageFlags    dstStageMask = 0;

    typedef struct alignas(32) EffectContext {

        // Effect identifier -- used for various resource callbacks to application
        FfxEffect           effectId;

        // Resource allocation
        uint32_t              nextStaticResource;
        uint32_t              nextDynamicResource;

        // UAV offsets
        uint32_t              nextStaticResourceView;
        uint32_t              nextDynamicResourceView[FFX_MAX_QUEUED_FRAMES];

        // Bindless descriptors
        uint32_t              bindlessTextureSrvHeapStart;
        uint32_t              bindlessTextureSrvHeapSize;
        uint32_t              bindlessBufferSrvHeapSize;
        uint32_t              bindlessTextureUavHeapStart;
        uint32_t              bindlessTextureUavHeapSize;
        uint32_t              bindlessBufferUavHeapSize;
        VkDescriptorPool      bindlessDescriptorPool;
        VkDescriptorSetLayout bindlessTextureSrvDescriptorSetLayout;
        VkDescriptorSetLayout bindlessBufferSrvDescriptorSetLayout;
        VkDescriptorSetLayout bindlessTextureUavDescriptorSetLayout;
        VkDescriptorSetLayout bindlessBufferUavDescriptorSetLayout;
        VkDescriptorSet       bindlessTextureSrvDescriptorSet;
        VkDescriptorSet       bindlessBufferSrvDescriptorSet;
        VkDescriptorSet       bindlessTextureUavDescriptorSet;
        VkDescriptorSet       bindlessBufferUavDescriptorSet;

        // Pipeline layout
        uint32_t              nextPipelineLayout;

        // the frame index for the context
        uint32_t              frameIndex;

        // Usage
        bool                  active;

        // VRAM usage
        FfxEffectMemoryUsage vramUsage;

    } EffectContext;

    Resource*               pResources;
    EffectContext*          pEffectContexts;

     // Allocation defaults
    FfxConstantAllocation FallbackConstantAllocator(void* data, FfxUInt64 dataSize);
    VkDeviceMemory        uniformBufferMemory = VK_NULL_HANDLE;
    VkMemoryPropertyFlags uniformBufferMemoryProperties;
    VkDeviceSize          uniformBufferAlignment = 0;
    void*                 uniformBufferMem       = nullptr;
    VkBuffer              uniformBuffer          = VK_NULL_HANDLE;
    VkDeviceSize          uniformBufferSize      = 0;
    VkDeviceSize          uniformBufferOffset    = 0;
    std::mutex            uniformBufferMutex;

    uint32_t                numDeviceExtensions = 0;
    VkExtensionProperties*  extensionProperties = nullptr;

    typedef enum BreadcrumbsFlags
    {
        BREADCRUMBS_DEDICATED_MEMORY_ENABLED    = 0x01,
        BREADCRUMBS_BUFFER_MARKER_ENABLED       = 0x02,
        BREADCRUMBS_SYNCHRONIZATION2_ENABLED    = 0x04,
    } BreadcrumbsFlags;
    uint8_t                 breadcrumbsFlags = 0;
    uint32_t                breadcrumbsMemoryIndex = 0;

} BackendContext_VK;

FFX_API size_t ffxGetScratchMemorySizeVK(VkPhysicalDevice physicalDevice, size_t maxContexts)
{
    uint32_t numExtensions = 0;

    if (physicalDevice)
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExtensions, nullptr);

    uint32_t extensionPropArraySize = sizeof(VkExtensionProperties) * numExtensions;
    uint32_t gpuJobDescArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));
    uint32_t resourceViewArraySize = FFX_ALIGN_UP(((maxContexts * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2) + FFX_MAX_BINDLESS_DESCRIPTOR_COUNT) * sizeof(BackendContext_VK::VkResourceView), sizeof(uint32_t));
    uint32_t stagingRingBufferArraySize = FFX_ALIGN_UP(maxContexts * FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE, sizeof(uint32_t));
    uint32_t pipelineArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_PASS_COUNT * sizeof(BackendContext_VK::PipelineLayout), sizeof(uint32_t));
    uint32_t resourceArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_VK::Resource), sizeof(uint32_t));
    uint32_t contextArraySize = FFX_ALIGN_UP(maxContexts * sizeof(BackendContext_VK::EffectContext), sizeof(uint32_t));
    // EffectContext is explicitly over-aligned. The upstream scratch layout
    // only aligns the preceding arrays to four bytes, which can make the
    // EffectContext pointer misaligned and lets optimized compilers emit
    // faulting aligned stores. Reserve enough padding to align it regardless
    // of the caller's scratch-buffer address.
    constexpr size_t contextAlignmentPadding = alignof(BackendContext_VK::EffectContext) - 1;
    
    return FFX_ALIGN_UP(sizeof(BackendContext_VK) + extensionPropArraySize + gpuJobDescArraySize + resourceViewArraySize + stagingRingBufferArraySize +
                            pipelineArraySize + resourceArraySize + contextAlignmentPadding + contextArraySize,
                        sizeof(uint64_t));
}

// Create a FfxDevice from a VkDevice
FfxDevice ffxGetDeviceVK(VkDeviceContext* vkDeviceContext)
{
    sVkDeviceContext = *vkDeviceContext;
    return reinterpret_cast<FfxDevice>(&sVkDeviceContext);
}

FfxErrorCode ffxGetInterfaceVK(
    FfxInterface* backendInterface,
    FfxDevice device,
    void* scratchBuffer,
    size_t scratchBufferSize,
    size_t maxContexts)
{
    FFX_RETURN_ON_ERROR(
        backendInterface,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBuffer,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBufferSize >= ffxGetScratchMemorySizeVK(((VkDeviceContext*)device)->vkPhysicalDevice, maxContexts),
        FFX_ERROR_INSUFFICIENT_MEMORY);

    backendInterface->fpGetSDKVersion = GetSDKVersionVK;
    backendInterface->fpGetEffectGpuMemoryUsage = GetEffectGpuMemoryUsageVK;
    backendInterface->fpCreateBackendContext = CreateBackendContextVK;
    backendInterface->fpGetDeviceCapabilities = GetDeviceCapabilitiesVK;
    backendInterface->fpDestroyBackendContext = DestroyBackendContextVK;
    backendInterface->fpCreateResource = CreateResourceVK;
    backendInterface->fpDestroyResource = DestroyResourceVK;
    backendInterface->fpMapResource = MapResourceVK;
    backendInterface->fpUnmapResource = UnmapResourceVK;
    backendInterface->fpRegisterResource = RegisterResourceVK;
    backendInterface->fpGetResource = GetResourceVK;
    backendInterface->fpUnregisterResources = UnregisterResourcesVK;
    backendInterface->fpRegisterStaticResource = RegisterStaticResourceVK;
    backendInterface->fpGetResourceDescription = GetResourceDescriptionVK;
    backendInterface->fpStageConstantBufferDataFunc = StageConstantBufferDataVK;
    backendInterface->fpCreatePipeline = CreatePipelineVK;
    backendInterface->fpDestroyPipeline = DestroyPipelineVK;
    backendInterface->fpGetPermutationBlobByIndex = ffxGetPermutationBlobByIndex;
    backendInterface->fpScheduleGpuJob = ScheduleGpuJobVK;
    backendInterface->fpExecuteGpuJobs = ExecuteGpuJobsVK;
    backendInterface->fpBreadcrumbsAllocBlock = BreadcrumbsAllocBlockVK;
    backendInterface->fpBreadcrumbsFreeBlock = BreadcrumbsFreeBlockVK;
    backendInterface->fpBreadcrumbsWrite = BreadcrumbsWriteVK;
    backendInterface->fpBreadcrumbsPrintDeviceInfo = BreadcrumbsPrintDeviceInfoVK;
    backendInterface->fpRegisterConstantBufferAllocator = RegisterConstantBufferAllocatorVK;
    // This portable target intentionally excludes AMD's Win32-only frame
    // interpolation swapchain implementation. Presentation is a separate
    // module concern; the native Vulkan compute backend remains usable.
    backendInterface->fpSwapChainConfigureFrameGeneration = nullptr;

    // Memory assignments
    backendInterface->scratchBuffer = scratchBuffer;
    backendInterface->scratchBufferSize = scratchBufferSize;

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FFX_RETURN_ON_ERROR(!backendContext->refCount, FFX_ERROR_BACKEND_API_ERROR);

    // Clear everything out
    memset(backendContext, 0, sizeof(*backendContext));

    // Map the device
    backendInterface->device = device;

    // Assign the max number of contexts we'll be using
    backendContext->maxEffectContexts = (uint32_t)maxContexts;

    return FFX_OK;
}

FfxCommandList ffxGetCommandListVK(VkCommandBuffer cmdBuf)
{
    FFX_ASSERT(NULL != cmdBuf);
    return reinterpret_cast<FfxCommandList>(cmdBuf);
}

FfxPipeline ffxGetPipelineVK(VkPipeline pipeline)
{
    FFX_ASSERT(NULL != pipeline);
    return reinterpret_cast<FfxPipeline>(pipeline);
}

FfxResource ffxGetResourceVK(void* vkResource,
    FfxResourceDescription          ffxResDescription,
    const wchar_t* ffxResName,
    FfxResourceStates               state /*=FFX_RESOURCE_STATE_COMPUTE_READ*/)
{
    FfxResource resource = {};
    resource.resource = vkResource;
    resource.state = state;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (ffxResName) {
        wcscpy_s(resource.name, ffxResName);
    }
#endif

    return resource;
}

uint32_t findMemoryTypeIndex(
    VkPhysicalDevice physicalDevice,
    VkMemoryRequirements memRequirements,
    VkMemoryPropertyFlags requestedProperties,
    VkBool32 deviceCoherentMemoryEnabled,
    VkMemoryPropertyFlags& outProperties)
{
    FFX_ASSERT(NULL != physicalDevice);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    uint32_t bestCandidate = UINT32_MAX;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        const VkMemoryPropertyFlags memoryProperties = memProperties.memoryTypes[i].propertyFlags;

        // VK_AMD_device_coherent_memory exposes the physical memory types even
        // when its logical-device feature was not enabled. Vulkan explicitly
        // forbids allocating such a type in that case (VUID-02790).
        if (!deviceCoherentMemoryEnabled &&
            (memoryProperties & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD))
            continue;

        if ((memRequirements.memoryTypeBits & (1u << i)) &&
            (memoryProperties & requestedProperties) == requestedProperties) {

            // if just device-local memory is requested, make sure this is the invisible heap to prevent over-subscribing the local heap
            if (requestedProperties == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && (memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
                continue;

            bestCandidate = i;
            outProperties = memoryProperties;

            // if host-visible memory is requested, check for host coherency as well and if available, return immediately
            if ((requestedProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                return bestCandidate;
        }
    }

    return bestCandidate;
}

VkBufferUsageFlags ffxGetVKBufferUsageFlagsFromResourceUsage(FfxResourceUsage flags)
{
    VkBufferUsageFlags indirectBit = 0;

    if (FFX_CONTAINS_FLAG(flags, FFX_RESOURCE_USAGE_INDIRECT))
        indirectBit = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    if (FFX_CONTAINS_FLAG(flags, FFX_RESOURCE_USAGE_UAV))
        return indirectBit | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    else
        return indirectBit | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
}

VkImageType ffxGetVKImageTypeFromResourceType(FfxResourceType type)
{
    switch (type) {
    case(FFX_RESOURCE_TYPE_TEXTURE1D):
        return VK_IMAGE_TYPE_1D;
    case(FFX_RESOURCE_TYPE_TEXTURE2D):
        return VK_IMAGE_TYPE_2D;
    case(FFX_RESOURCE_TYPE_TEXTURE_CUBE):
    case(FFX_RESOURCE_TYPE_TEXTURE3D):
        return VK_IMAGE_TYPE_3D;
    default:
        return VK_IMAGE_TYPE_MAX_ENUM;
    }
}

bool ffxIsSurfaceFormatSRGB(FfxSurfaceFormat fmt)
{
    switch (fmt)
    {
    case (FFX_SURFACE_FORMAT_R8G8B8A8_SRGB):
    case (FFX_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return true;
    case (FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
    case (FFX_SURFACE_FORMAT_R32G32B32A32_UINT):
    case (FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
    case (FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
    case (FFX_SURFACE_FORMAT_R32G32B32_FLOAT):
    case (FFX_SURFACE_FORMAT_R32G32_FLOAT):
    case (FFX_SURFACE_FORMAT_R8_UINT):
    case (FFX_SURFACE_FORMAT_R32_UINT):
    case (FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
    case (FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
    case (FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
    case (FFX_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
    case (FFX_SURFACE_FORMAT_B8G8R8A8_UNORM):
    case (FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
    case (FFX_SURFACE_FORMAT_R10G10B10A2_UNORM):
    case (FFX_SURFACE_FORMAT_R16G16_FLOAT):
    case (FFX_SURFACE_FORMAT_R16G16_UINT):
    case (FFX_SURFACE_FORMAT_R16G16_SINT):
    case (FFX_SURFACE_FORMAT_R16_FLOAT):
    case (FFX_SURFACE_FORMAT_R16_UINT):
    case (FFX_SURFACE_FORMAT_R16_UNORM):
    case (FFX_SURFACE_FORMAT_R16_SNORM):
    case (FFX_SURFACE_FORMAT_R8_UNORM):
    case (FFX_SURFACE_FORMAT_R8G8_UNORM):
    case (FFX_SURFACE_FORMAT_R8G8_UINT):
    case (FFX_SURFACE_FORMAT_R32_FLOAT):
    case (FFX_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP):
    case (FFX_SURFACE_FORMAT_UNKNOWN):
        return false;
    default:
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return false;
    }
}

FfxSurfaceFormat ffxGetSurfaceFormatFromGamma(FfxSurfaceFormat fmt)
{
    switch (fmt)
    {
    case (FFX_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case (FFX_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return FFX_SURFACE_FORMAT_B8G8R8A8_UNORM;
    default:
        return fmt;
    }
}

FfxSurfaceFormat ffxGetSurfaceFormatToGamma(FfxSurfaceFormat fmt)
{
    switch (fmt)
    {
    case (FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
        return FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
    case (FFX_SURFACE_FORMAT_B8G8R8A8_UNORM):
        return FFX_SURFACE_FORMAT_B8G8R8A8_SRGB;
    default:
        return fmt;
    }
}

VkFormat ffxGetVkFormatFromSurfaceFormat(FfxSurfaceFormat fmt)
{
    switch (fmt)
    {
    case(FFX_SURFACE_FORMAT_UNKNOWN):
        return VK_FORMAT_UNDEFINED;
    case(FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32G32B32A32_UINT):
        return VK_FORMAT_R32G32B32A32_UINT;
    case(FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case (FFX_SURFACE_FORMAT_R32G32B32_FLOAT):
        return VK_FORMAT_R32G32B32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32G32_FLOAT):
        return VK_FORMAT_R32G32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R8_UINT):
        return VK_FORMAT_R8_UINT;
    case(FFX_SURFACE_FORMAT_R32_UINT):
        return VK_FORMAT_R32_UINT;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case (FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
        return VK_FORMAT_R8G8B8A8_SNORM;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return VK_FORMAT_R8G8B8A8_SRGB;
    case(FFX_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case(FFX_SURFACE_FORMAT_B8G8R8A8_UNORM):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case (FFX_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return VK_FORMAT_B8G8R8A8_SRGB;
    case(FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case (FFX_SURFACE_FORMAT_R10G10B10A2_UNORM):
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case(FFX_SURFACE_FORMAT_R16G16_FLOAT):
        return VK_FORMAT_R16G16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16G16_UINT):
        return VK_FORMAT_R16G16_UINT;
    case(FFX_SURFACE_FORMAT_R16G16_SINT):
        return VK_FORMAT_R16G16_SINT;
    case(FFX_SURFACE_FORMAT_R16_FLOAT):
        return VK_FORMAT_R16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16_UINT):
        return VK_FORMAT_R16_UINT;
    case(FFX_SURFACE_FORMAT_R16_UNORM):
        return VK_FORMAT_R16_UNORM;
    case(FFX_SURFACE_FORMAT_R16_SNORM):
        return VK_FORMAT_R16_SNORM;
    case(FFX_SURFACE_FORMAT_R8_UNORM):
        return VK_FORMAT_R8_UNORM;
    case(FFX_SURFACE_FORMAT_R8G8_UNORM):
        return VK_FORMAT_R8G8_UNORM;
    case(FFX_SURFACE_FORMAT_R8G8_UINT):
        return VK_FORMAT_R8G8_UINT;
    case(FFX_SURFACE_FORMAT_R32_FLOAT):
        return VK_FORMAT_R32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP):
        return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;

    default:
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return VK_FORMAT_UNDEFINED;
    }
}

VkFormat ffxGetVKUAVFormatFromSurfaceFormat(FfxSurfaceFormat fmt)
{
    switch (fmt)
    {
    case(FFX_SURFACE_FORMAT_UNKNOWN):
        return VK_FORMAT_UNDEFINED;
    case(FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32G32B32A32_UINT):
        return VK_FORMAT_R32G32B32A32_UINT;
    case(FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32G32B32_FLOAT):
        return VK_FORMAT_R32G32B32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R32G32_FLOAT):
        return VK_FORMAT_R32G32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R8_UINT):
        return VK_FORMAT_R8_UINT;
    case(FFX_SURFACE_FORMAT_R32_UINT):
        return VK_FORMAT_R32_UINT;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
    case(FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
    case(FFX_SURFACE_FORMAT_R8G8B8A8_SRGB):
        return VK_FORMAT_R8G8B8A8_UNORM;
    case(FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
        return VK_FORMAT_R8G8B8A8_SNORM;
    case(FFX_SURFACE_FORMAT_B8G8R8A8_TYPELESS):
    case(FFX_SURFACE_FORMAT_B8G8R8A8_UNORM):
    case (FFX_SURFACE_FORMAT_B8G8R8A8_SRGB):
        return VK_FORMAT_B8G8R8A8_UNORM;
    case(FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case (FFX_SURFACE_FORMAT_R10G10B10A2_UNORM):
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case(FFX_SURFACE_FORMAT_R16G16_FLOAT):
        return VK_FORMAT_R16G16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16G16_UINT):
        return VK_FORMAT_R16G16_UINT;
    case(FFX_SURFACE_FORMAT_R16G16_SINT):
        return VK_FORMAT_R16G16_SINT;
    case(FFX_SURFACE_FORMAT_R16_FLOAT):
        return VK_FORMAT_R16_SFLOAT;
    case(FFX_SURFACE_FORMAT_R16_UINT):
        return VK_FORMAT_R16_UINT;
    case(FFX_SURFACE_FORMAT_R16_UNORM):
        return VK_FORMAT_R16_UNORM;
    case(FFX_SURFACE_FORMAT_R16_SNORM):
        return VK_FORMAT_R16_SNORM;
    case(FFX_SURFACE_FORMAT_R8_UNORM):
        return VK_FORMAT_R8_UNORM;
    case(FFX_SURFACE_FORMAT_R8G8_UNORM):
        return VK_FORMAT_R8G8_UNORM;
    case(FFX_SURFACE_FORMAT_R8G8_UINT):
        return VK_FORMAT_R8G8_UINT;
    case(FFX_SURFACE_FORMAT_R32_FLOAT):
        return VK_FORMAT_R32_SFLOAT;
    case(FFX_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP):
        return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;

    default:
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return VK_FORMAT_UNDEFINED;
    }
}

FfxSurfaceFormat ffxGetSurfaceFormatVK(VkFormat fmt)
{
    switch (fmt)
    {
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    case VK_FORMAT_R32G32B32A32_UINT:
        return FFX_SURFACE_FORMAT_R32G32B32A32_UINT;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    case VK_FORMAT_R32G32_SFLOAT:
        return FFX_SURFACE_FORMAT_R32G32_FLOAT;
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return FFX_SURFACE_FORMAT_R32_UINT;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_R8G8B8A8_SNORM:
        return FFX_SURFACE_FORMAT_R8G8B8A8_SNORM;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return FFX_SURFACE_FORMAT_B8G8R8A8_UNORM;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return FFX_SURFACE_FORMAT_B8G8R8A8_SRGB;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;
    case VK_FORMAT_R16G16_SFLOAT:
        return FFX_SURFACE_FORMAT_R16G16_FLOAT;
    case VK_FORMAT_R16G16_UINT:
        return FFX_SURFACE_FORMAT_R16G16_UINT;
    case VK_FORMAT_R16G16_SINT:
        return FFX_SURFACE_FORMAT_R16G16_SINT;
    case VK_FORMAT_R16_SFLOAT:
        return FFX_SURFACE_FORMAT_R16_FLOAT;
    case VK_FORMAT_R16_UINT:
        return FFX_SURFACE_FORMAT_R16_UINT;
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D16_UNORM_S8_UINT:
        return FFX_SURFACE_FORMAT_R16_UNORM;
    case VK_FORMAT_R16_SNORM:
        return FFX_SURFACE_FORMAT_R16_SNORM;
    case VK_FORMAT_R8_UNORM:
        return FFX_SURFACE_FORMAT_R8_UNORM;
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_S8_UINT:
        return FFX_SURFACE_FORMAT_R8_UINT;
    case VK_FORMAT_R8G8_UNORM:
        return FFX_SURFACE_FORMAT_R8G8_UNORM;
    case VK_FORMAT_R8G8_UINT:
        return FFX_SURFACE_FORMAT_R8G8_UINT;
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return FFX_SURFACE_FORMAT_R32_FLOAT;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        return FFX_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP;
    case VK_FORMAT_UNDEFINED:
        return FFX_SURFACE_FORMAT_UNKNOWN;

    default:
        // NOTE: we do not support typeless formats here
        FFX_ASSERT_MESSAGE(false, "Format not yet supported");
        return FFX_SURFACE_FORMAT_UNKNOWN;
    }
}

bool isDepthFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;

    default:
        return false;
    }
}

bool isStencilFormat(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;

    default:
        return false;
    }
}

VkImageAspectFlags getImageAspect(FfxResourceUsage usage)
{
    // No VK_IMAGE_ASPECT_STENCIL_BIT support for now
    if (FFX_CONTAINS_FLAG(usage, FFX_RESOURCE_USAGE_DEPTHTARGET))
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkFormat getVkFormatFromSurfaceFormatAndUsage(FfxSurfaceFormat fmt, FfxResourceUsage usage)
{
    VkFormat format = ffxGetVkFormatFromSurfaceFormat(fmt);
    bool hasDepth = FFX_CONTAINS_FLAG(usage, FFX_RESOURCE_USAGE_DEPTHTARGET);
    bool hasStencil = FFX_CONTAINS_FLAG(usage, FFX_RESOURCE_USAGE_STENCILTARGET);
    if (hasDepth || hasStencil)
    {
        switch (fmt)
        {
        case FFX_SURFACE_FORMAT_R16_UNORM:
        {
            if (hasDepth && hasStencil)
                return VK_FORMAT_D16_UNORM_S8_UINT;
            else if (hasDepth)
                return VK_FORMAT_D16_UNORM;
            // stencil only doesn't exist
            break;
        }
        case FFX_SURFACE_FORMAT_R32_FLOAT:
        {
            if (hasDepth && hasStencil)
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
            else if (hasDepth)
                return VK_FORMAT_D32_SFLOAT;
            // stencil only doesn't exist
            break;
        }
        case FFX_SURFACE_FORMAT_R32_UINT:
        {
            if (hasDepth && hasStencil)
                return VK_FORMAT_D24_UNORM_S8_UINT;
            else if (hasDepth)
                return VK_FORMAT_X8_D24_UNORM_PACK32;
            // stencil only doesn't exist
            break;
        }
        case FFX_SURFACE_FORMAT_R8_UINT:
        {
            if (hasStencil)
                return VK_FORMAT_S8_UINT;
            break;
        }
        default:
            break;
        }
    }

    return format;
}


VkImageUsageFlags getVKImageUsageFlagsFromResourceUsage(FfxResourceUsage flags)
{
    VkImageUsageFlags ret = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (FFX_CONTAINS_FLAG(flags, FFX_RESOURCE_USAGE_RENDERTARGET)) ret |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (FFX_CONTAINS_FLAG(flags, FFX_RESOURCE_USAGE_UAV)) ret |= (VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    return ret;
}

FfxResourceDescription ffxGetBufferResourceDescriptionVK(const VkBuffer buffer, const VkBufferCreateInfo createInfo, FfxResourceUsage additionalUsages /*=FFX_RESOURCE_USAGE_READ_ONLY*/)
{
    FfxResourceDescription resourceDescription = {};

    // This is valid
    if (buffer == VK_NULL_HANDLE)
        return resourceDescription;

    resourceDescription.flags  = FFX_RESOURCE_FLAGS_NONE;
    resourceDescription.usage  = additionalUsages;
    resourceDescription.size   = (uint32_t)createInfo.size;
    resourceDescription.stride = 0;
    resourceDescription.format = FFX_SURFACE_FORMAT_UNKNOWN;

    if (FFX_CONTAINS_FLAG(createInfo.usage, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
        resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_UAV);
    if (FFX_CONTAINS_FLAG(createInfo.usage, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT))
        resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_INDIRECT);

    // What should we initialize this to?? No case for this yet
    resourceDescription.depth    = 0;
    resourceDescription.mipCount = 0;

    // Set the type
