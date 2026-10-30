    resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
    
    return resourceDescription;
}

FfxResourceDescription ffxGetImageResourceDescriptionVK(const VkImage           image,
                                                        const VkImageCreateInfo createInfo,
                                                        FfxResourceUsage        additionalUsages /*=FFX_RESOURCE_USAGE_READ_ONLY*/)
{
    FfxResourceDescription resourceDescription = {};

    // This is valid
    if (image == VK_NULL_HANDLE)
        return resourceDescription;

    // Set flags properly for resource registration
    resourceDescription.flags = FFX_RESOURCE_FLAGS_NONE;
    resourceDescription.usage = FFX_RESOURCE_USAGE_READ_ONLY;

    // Check for depth and stencil use
    if (isDepthFormat(createInfo.format))
        resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_DEPTHTARGET);
    if (isStencilFormat(createInfo.format))
        resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_STENCILTARGET);

    // Unordered access use
    if (FFX_CONTAINS_FLAG(createInfo.usage, VK_IMAGE_USAGE_STORAGE_BIT))
        resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_UAV);

    // Resource-specific supplemental use flags
    resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | additionalUsages);

    resourceDescription.width    = createInfo.extent.width;
    resourceDescription.height   = createInfo.extent.height;
    resourceDescription.mipCount = createInfo.mipLevels;
    resourceDescription.format   = ffxGetSurfaceFormatVK(createInfo.format);

    // if the mutable flag is present, assume that the real format is sRGB
    if (FFX_CONTAINS_FLAG(createInfo.flags, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT))
        resourceDescription.format = ffxGetSurfaceFormatToGamma(resourceDescription.format);
    
    switch (createInfo.imageType)
    {
    case VK_IMAGE_TYPE_1D:
        resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE1D;
        break;
    case VK_IMAGE_TYPE_2D:
        resourceDescription.depth = createInfo.arrayLayers;
        if (FFX_CONTAINS_FLAG(additionalUsages, FFX_RESOURCE_USAGE_ARRAYVIEW))
            resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
        else if (FFX_CONTAINS_FLAG(createInfo.flags, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT))
            resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE_CUBE;
        else
            resourceDescription.type  = FFX_RESOURCE_TYPE_TEXTURE2D;
        break;
    case VK_IMAGE_TYPE_3D:
        resourceDescription.depth = createInfo.extent.depth;
        resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE3D;
        break;
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: VK: Unsupported texture dimension requested. Please implement.");
        break;
    }

    return resourceDescription;
}

FfxErrorCode allocateDeviceMemory(BackendContext_VK* backendContext, VkMemoryRequirements memRequirements, VkMemoryPropertyFlags requiredMemoryProperties, BackendContext_VK::Resource* backendResource)
{
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryTypeIndex(
        backendContext->physicalDevice,
        memRequirements,
        requiredMemoryProperties,
        backendContext->deviceCoherentMemoryEnabled,
        backendResource->memoryProperties);

    if (allocInfo.memoryTypeIndex == UINT32_MAX) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    VkResult result = backendContext->vkFunctionTable.vkAllocateMemory(backendContext->device, &allocInfo, nullptr, &backendResource->deviceMemory);

    if (result != VK_SUCCESS) {
        switch (result) {
        case(VK_ERROR_OUT_OF_HOST_MEMORY):
        case(VK_ERROR_OUT_OF_DEVICE_MEMORY):
            return FFX_ERROR_OUT_OF_MEMORY;
        default:
            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }

    return FFX_OK;
}

void setVKObjectName(BackendContext_VK::VKFunctionTable& vkFunctionTable, VkDevice device, VkObjectType objectType, uint64_t object, char* name)
{
    VkDebugUtilsObjectNameInfoEXT s{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, objectType, object, name };

    if (vkFunctionTable.vkSetDebugUtilsObjectNameEXT)
        vkFunctionTable.vkSetDebugUtilsObjectNameEXT(device, &s);
}

uint32_t getDynamicResourcesStartIndex(uint32_t effectContextId)
{
    // dynamic resources are tracked from the max index
    return (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
}
uint32_t getDynamicResourceViewsStartIndex(uint32_t effectContextId, uint32_t frameIndex)
{
    // dynamic resource views are tracked from the max index
    return (effectContextId * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2) + (frameIndex * FFX_MAX_RESOURCE_COUNT * 2) + (FFX_MAX_RESOURCE_COUNT * 2) - 1;
}

void destroyDynamicViews(BackendContext_VK* backendContext, uint32_t effectContextId, uint32_t frameIndex)
{
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Release image views for dynamic resources
    const uint32_t dynamicResourceViewIndexStart = getDynamicResourceViewsStartIndex(effectContextId, frameIndex);
    for (uint32_t dynamicViewIndex = effectContext.nextDynamicResourceView[frameIndex] + 1; dynamicViewIndex <= dynamicResourceViewIndexStart;
         ++dynamicViewIndex)
    {
        backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[dynamicViewIndex].imageView, VK_NULL_HANDLE);
        backendContext->pResourceViews[dynamicViewIndex].imageView = VK_NULL_HANDLE;
    }
    effectContext.nextDynamicResourceView[frameIndex] = dynamicResourceViewIndexStart;
}

VkAccessFlags getVKAccessFlagsFromResourceState(FfxResourceStates state)
{
    switch (state) {

    case(FFX_RESOURCE_STATE_COMMON):
        return VK_ACCESS_NONE;
    case(FFX_RESOURCE_STATE_GENERIC_READ):
        return VK_ACCESS_SHADER_READ_BIT;
    case(FFX_RESOURCE_STATE_UNORDERED_ACCESS):
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    case(FFX_RESOURCE_STATE_COMPUTE_READ):
    case(FFX_RESOURCE_STATE_PIXEL_READ):
    case(FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ):
        return VK_ACCESS_SHADER_READ_BIT;
    case FFX_RESOURCE_STATE_COPY_SRC:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case FFX_RESOURCE_STATE_COPY_DEST:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case FFX_RESOURCE_STATE_INDIRECT_ARGUMENT:
        return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    case FFX_RESOURCE_STATE_PRESENT:
        return VK_ACCESS_NONE;
    case FFX_RESOURCE_STATE_RENDER_TARGET:
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    case FFX_RESOURCE_STATE_DEPTH_ATTACHEMENT:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    default:
        FFX_ASSERT_MESSAGE(false, "State flag not yet supported");
        return VK_ACCESS_SHADER_READ_BIT;
    }
}

VkPipelineStageFlags getVKPipelineStageFlagsFromResourceState(FfxResourceStates state)
{
    switch (state) {

    case(FFX_RESOURCE_STATE_COMMON):
    case(FFX_RESOURCE_STATE_GENERIC_READ):
    case(FFX_RESOURCE_STATE_UNORDERED_ACCESS):
    case(FFX_RESOURCE_STATE_COMPUTE_READ):
    case(FFX_RESOURCE_STATE_PIXEL_READ):
    case(FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ):
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    case(FFX_RESOURCE_STATE_INDIRECT_ARGUMENT):
        return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
    case FFX_RESOURCE_STATE_COPY_SRC:
    case FFX_RESOURCE_STATE_COPY_DEST:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
    case FFX_RESOURCE_STATE_PRESENT:
        return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    case FFX_RESOURCE_STATE_RENDER_TARGET:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    case FFX_RESOURCE_STATE_DEPTH_ATTACHEMENT:
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    default:
        FFX_ASSERT_MESSAGE(false, "Pipeline stage flag not yet supported");
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
}

VkImageLayout getVKImageLayoutFromResourceState(FfxResourceStates state)
{
    switch (state) {

    case(FFX_RESOURCE_STATE_COMMON):
        return VK_IMAGE_LAYOUT_GENERAL;
    case(FFX_RESOURCE_STATE_GENERIC_READ):
        return VK_IMAGE_LAYOUT_GENERAL;
    case(FFX_RESOURCE_STATE_UNORDERED_ACCESS):
        return VK_IMAGE_LAYOUT_GENERAL;
    case(FFX_RESOURCE_STATE_COMPUTE_READ):
    case(FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ):
    case(FFX_RESOURCE_STATE_PIXEL_READ):
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case FFX_RESOURCE_STATE_COPY_SRC:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case FFX_RESOURCE_STATE_COPY_DEST:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case FFX_RESOURCE_STATE_PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    case FFX_RESOURCE_STATE_RENDER_TARGET:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case FFX_RESOURCE_STATE_DEPTH_ATTACHEMENT:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case FFX_RESOURCE_STATE_INDIRECT_ARGUMENT:
        // this case is for buffers
    default:
        FFX_ASSERT_MESSAGE(false, "Image layout flag not yet supported");
        return VK_IMAGE_LAYOUT_GENERAL;
    }
}

void addMutableViewForSRV(VkImageViewCreateInfo& imageViewCreateInfo, VkImageViewUsageCreateInfo& imageViewUsageCreateInfo, FfxResourceDescription resourceDescription)
{
    if (ffxIsSurfaceFormatSRGB(resourceDescription.format) && FFX_CONTAINS_FLAG(resourceDescription.usage, FFX_RESOURCE_USAGE_UAV))
    {
        // mutable is only for sRGB textures that will need a storage
        imageViewUsageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
        imageViewUsageCreateInfo.pNext = nullptr;
        imageViewUsageCreateInfo.usage = getVKImageUsageFlagsFromResourceUsage(FFX_RESOURCE_USAGE_READ_ONLY);  // we can assume that SRV is enough
        imageViewCreateInfo.pNext      = &imageViewUsageCreateInfo;
    }
}

void copyResourceState(BackendContext_VK::Resource* backendResource, const FfxResource* inFfxResource)
{
    FfxResourceStates state = inFfxResource->state;

    // copy the new states
    backendResource->initialState = state;
    backendResource->currentState = state;
    backendResource->undefined    = false;
    backendResource->dynamic      = true;

    // If the internal resource state is undefined, that means we are importing a resource that
    // has not yet been initialized, so tag the resource as undefined so we can transition it accordingly.
    if (backendResource->resourceDescription.flags & FFX_RESOURCE_FLAGS_UNDEFINED)
    {
        backendResource->undefined                 = true;
        backendResource->resourceDescription.flags = (FfxResourceFlags)((int)backendResource->resourceDescription.flags & ~FFX_RESOURCE_FLAGS_UNDEFINED);
    }
}

#ifdef _WIN32
void ConvertUTF8ToUTF16(const char* inputName, wchar_t* outputBuffer, size_t outputLen)
{
    if (MultiByteToWideChar(CP_UTF8, 0, inputName, -1, outputBuffer, static_cast<int>(outputLen)) == 0)
    {
        memset(outputBuffer, 0, outputLen * sizeof(wchar_t));
    }
}
void ConvertUTF16ToUTF8(const wchar_t* inputName, char* outputBuffer, size_t outputLen)
{
    if (WideCharToMultiByte(CP_UTF8, 0, inputName, -1, outputBuffer, static_cast<int>(outputLen * sizeof(char)), NULL, NULL) == 0)
    {
        memset(outputBuffer, 0, outputLen * sizeof(char));
    }
}
#else
void ConvertUTF8ToUTF16(const char* inputName, wchar_t* outputBuffer, size_t outputLen)
{
    memset(outputBuffer, 0, outputLen * sizeof(wchar_t));
    ffxPortableUtf8ToWide(inputName, outputBuffer, outputLen);
}
void ConvertUTF16ToUTF8(const wchar_t* inputName, char* outputBuffer, size_t outputLen)
{
    memset(outputBuffer, 0, outputLen * sizeof(char));
    ffxPortableWideToUtf8(inputName, outputBuffer, outputLen);
}
#endif  // _WIN32

void beginMarkerVK(BackendContext_VK* backendContext, VkCommandBuffer commandBuffer, const wchar_t* label)
{
    if(backendContext->vkFunctionTable.vkCmdBeginDebugUtilsLabelEXT == nullptr)
    {
        return;
    }

    constexpr size_t strLen = 64;
    char strLabel[strLen];
    ConvertUTF16ToUTF8(label, strLabel, strLen);

    VkDebugUtilsLabelEXT debugLabel = {};
    debugLabel.sType                = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    debugLabel.pNext                = nullptr;
    debugLabel.pLabelName           = strLabel;

    // not to saturated red
    debugLabel.color[0] = 1.0f;
    debugLabel.color[1] = 0.14f;
    debugLabel.color[2] = 0.14f;
    debugLabel.color[3] = 1.0f;
    backendContext->vkFunctionTable.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &debugLabel);
}

void endMarkerVK(BackendContext_VK* backendContext, VkCommandBuffer commandBuffer)
{
    if(backendContext->vkFunctionTable.vkCmdBeginDebugUtilsLabelEXT != nullptr)
    {
        backendContext->vkFunctionTable.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
    }
}

void addBarrier(BackendContext_VK* backendContext, FfxResourceInternal* resource, FfxResourceStates newState)
{
    FFX_ASSERT(NULL != backendContext);
    FFX_ASSERT(NULL != resource);

    BackendContext_VK::Resource& ffxResource = backendContext->pResources[resource->internalIndex];

    if (ffxResource.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResource = ffxResource.bufferResource;
        VkBufferMemoryBarrier* barrier = &backendContext->bufferMemoryBarriers[backendContext->scheduledBufferBarrierCount];

        FfxResourceStates& curState = backendContext->pResources[resource->internalIndex].currentState;

        barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier->pNext = nullptr;
        barrier->srcAccessMask = getVKAccessFlagsFromResourceState(curState);
        barrier->dstAccessMask = getVKAccessFlagsFromResourceState(newState);
        barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier->buffer = vkResource;
        barrier->offset = 0;
        barrier->size = VK_WHOLE_SIZE;

        backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(curState);
        backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(newState);

        curState = newState;

        ++backendContext->scheduledBufferBarrierCount;
    }
    else
    {
        VkImage vkResource = ffxResource.imageResource;
        VkImageMemoryBarrier* barrier = &backendContext->imageMemoryBarriers[backendContext->scheduledImageBarrierCount];

        FfxResourceStates& curState = backendContext->pResources[resource->internalIndex].currentState;

        VkImageSubresourceRange range;
        range.aspectMask = getImageAspect(ffxResource.resourceDescription.usage);
        range.baseMipLevel = 0;
        range.levelCount = VK_REMAINING_MIP_LEVELS;
        range.baseArrayLayer = 0;
        range.layerCount = VK_REMAINING_ARRAY_LAYERS;

        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier->pNext = nullptr;
        barrier->srcAccessMask = getVKAccessFlagsFromResourceState(curState);
        barrier->dstAccessMask = getVKAccessFlagsFromResourceState(newState);
        barrier->oldLayout = ffxResource.undefined ? VK_IMAGE_LAYOUT_UNDEFINED : getVKImageLayoutFromResourceState(curState);
        barrier->newLayout = getVKImageLayoutFromResourceState(newState);
        barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier->image = vkResource;
        barrier->subresourceRange = range;

        backendContext->srcStageMask |= getVKPipelineStageFlagsFromResourceState(curState);
        backendContext->dstStageMask |= getVKPipelineStageFlagsFromResourceState(newState);

        curState = newState;

        ++backendContext->scheduledImageBarrierCount;
    }

    if (ffxResource.undefined)
        ffxResource.undefined = false;
}

void flushBarriers(BackendContext_VK* backendContext, VkCommandBuffer vkCommandBuffer)
{
    FFX_ASSERT(NULL != backendContext);
    FFX_ASSERT(NULL != vkCommandBuffer);

    if (backendContext->scheduledImageBarrierCount > 0 || backendContext->scheduledBufferBarrierCount > 0)
    {
        backendContext->vkFunctionTable.vkCmdPipelineBarrier(vkCommandBuffer, backendContext->srcStageMask, backendContext->dstStageMask, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, backendContext->scheduledBufferBarrierCount, backendContext->bufferMemoryBarriers, backendContext->scheduledImageBarrierCount, backendContext->imageMemoryBarriers);
        backendContext->scheduledImageBarrierCount = 0;
        backendContext->scheduledBufferBarrierCount = 0;
        backendContext->srcStageMask = 0;
        backendContext->dstStageMask = 0;
    }
}

FfxConstantAllocation BackendContext_VK::FallbackConstantAllocator(void* data, FfxUInt64 dataSize)
{
    FfxConstantAllocation       allocation;
    std::lock_guard<std::mutex> cbLock{uniformBufferMutex};

    if (!uniformBufferMem)
    {
        // allocate dynamic uniform buffer

        // get alignment
        VkPhysicalDeviceProperties physicalDeviceProperties = {};
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        uniformBufferAlignment = physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

        uniformBufferSize = FFX_ALIGN_UP(FFX_BUFFER_SIZE, uniformBufferAlignment) * maxEffectContexts * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES;

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size               = static_cast<VkDeviceSize>(uniformBufferSize);
        bufferInfo.usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

        VkResult res = vkFunctionTable.vkCreateBuffer(device, &bufferInfo, NULL, &uniformBuffer);
        FFX_ASSERT(res == VK_SUCCESS);

        VkMemoryAllocateInfo allocInfo = {};
        if (res == VK_SUCCESS)
        {
            // allocate memory block for all uniform buffers
            VkMemoryRequirements memRequirements = {};
            vkFunctionTable.vkGetBufferMemoryRequirements(device, uniformBuffer, &memRequirements);

            // this is the real alignment
            uniformBufferAlignment = memRequirements.alignment;

            VkMemoryPropertyFlags requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize  = uniformBufferSize;
            allocInfo.memoryTypeIndex = findMemoryTypeIndex(
                physicalDevice,
                memRequirements,
                requiredMemoryProperties,
                deviceCoherentMemoryEnabled,
                uniformBufferMemoryProperties);

            if (allocInfo.memoryTypeIndex == UINT32_MAX)
            {
                requiredMemoryProperties  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                allocInfo.memoryTypeIndex = findMemoryTypeIndex(
                    physicalDevice,
                    memRequirements,
                    requiredMemoryProperties,
                    deviceCoherentMemoryEnabled,
                    uniformBufferMemoryProperties);

                if (allocInfo.memoryTypeIndex == UINT32_MAX)
                    res = VK_ERROR_INITIALIZATION_FAILED;
            }
        }

        if (res == VK_SUCCESS)
            res = vkFunctionTable.vkAllocateMemory(device, &allocInfo, nullptr, &uniformBufferMemory);
        FFX_ASSERT(res == VK_SUCCESS);

        if (res == VK_SUCCESS)
            res = vkFunctionTable.vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory, 0);
        FFX_ASSERT(res == VK_SUCCESS);

        // map the memory block
        if (res == VK_SUCCESS)
            res = vkFunctionTable.vkMapMemory(device, uniformBufferMemory, 0, uniformBufferSize, 0, &uniformBufferMem);
        FFX_ASSERT(res == VK_SUCCESS);
    }

    FFX_ASSERT(uniformBufferMem);

    allocation.resource.resource = uniformBuffer;
    allocation.handle            = 0;

    if (data)
    {
        if (uniformBufferOffset + dataSize >= uniformBufferSize)
            uniformBufferOffset = 0;

        allocation.handle = static_cast<FfxUInt64>(uniformBufferOffset);

        void* pBuffer = (void*)((uint8_t*)(uniformBufferMem) + uniformBufferOffset);
        memcpy(pBuffer, data, dataSize);

        // TODO: ensure that we aren't writing on some used memory

        // flush mapped range if memory type is not coherent
        if ((uniformBufferMemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            VkMappedMemoryRange memoryRange;
            memset(&memoryRange, 0, sizeof(memoryRange));

            memoryRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRange.memory = uniformBufferMemory;
            memoryRange.offset = uniformBufferOffset;
            memoryRange.size   = dataSize;

            vkFunctionTable.vkFlushMappedMemoryRanges(device, 1, &memoryRange);
        }

        uniformBufferOffset += FFX_ALIGN_UP(dataSize, uniformBufferAlignment);
        if (uniformBufferOffset > uniformBufferSize)
            uniformBufferOffset = 0;
    }

    return allocation;
}

void resetBackendContext(BackendContext_VK* backendContext)
{
    // reset the context except the maxEffectContexts in case the memory is reused for a new context
    uint32_t maxEffectContexts = backendContext->maxEffectContexts;

    memset(backendContext, 0, sizeof(BackendContext_VK));

    // restore the maxEffectContexts
    backendContext->maxEffectContexts = maxEffectContexts;
}

//////////////////////////////////////////////////////////////////////////
// VK back end implementation

FfxVersionNumber GetSDKVersionVK(FfxInterface* backendInterface)
{
    return FFX_SDK_MAKE_VERSION(FFX_SDK_VERSION_MAJOR, FFX_SDK_VERSION_MINOR, FFX_SDK_VERSION_PATCH);
}

FfxErrorCode GetEffectGpuMemoryUsageVK(FfxInterface* backendInterface, FfxUInt32 effectContextId, FfxEffectMemoryUsage* outVramUsage)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != outVramUsage);

    BackendContext_VK*                backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    *outVramUsage = effectContext.vramUsage;

    return FFX_OK;
}

FfxErrorCode CreateBackendContextVK(FfxInterface* backendInterface, FfxEffect effect, FfxEffectBindlessConfig* bindlessConfig, FfxUInt32* effectContextId)
{
    VkDeviceContext* vkDeviceContext = reinterpret_cast<VkDeviceContext*>(backendInterface->device);

    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != vkDeviceContext);
    FFX_ASSERT(VK_NULL_HANDLE != vkDeviceContext->vkDevice);
    FFX_ASSERT(VK_NULL_HANDLE != vkDeviceContext->vkPhysicalDevice);

    // set up some internal resources we need (space for resource views and constant buffers)
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    // Set things up if this is the first invocation
    if (!backendContext->refCount) {

        resetBackendContext(backendContext);

        new (&backendContext->uniformBufferMutex) std::mutex();

        // Map all of our pointers
        uint32_t gpuJobDescArraySize   = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));
        uint32_t resourceViewArraySize = FFX_ALIGN_UP(((backendContext->maxEffectContexts * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2) + FFX_MAX_BINDLESS_DESCRIPTOR_COUNT) * sizeof(BackendContext_VK::VkResourceView), sizeof(uint32_t));
        uint32_t stagingRingBufferArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE, sizeof(uint32_t));
        uint32_t pipelineArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_MAX_PASS_COUNT * sizeof(BackendContext_VK::PipelineLayout), sizeof(uint32_t));
        uint32_t resourceArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_VK::Resource), sizeof(uint32_t));
        uint32_t contextArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * sizeof(BackendContext_VK::EffectContext), sizeof(uint32_t));
        uint8_t* pMem = (uint8_t*)((BackendContext_VK*)(backendContext + 1));

        // Map gpu job array
        backendContext->pGpuJobs = (FfxGpuJobDescription*)pMem;
        memset(backendContext->pGpuJobs, 0, gpuJobDescArraySize);
        pMem += gpuJobDescArraySize;

        // Map the resource view array
        backendContext->pResourceViews = (BackendContext_VK::VkResourceView*)(pMem);
        memset(backendContext->pResourceViews, 0, resourceViewArraySize);
        pMem += resourceViewArraySize;

        // Map the staging ring buffer array
        backendContext->pStagingRingBuffer = (uint8_t*)pMem;
        memset(backendContext->pStagingRingBuffer, 0, stagingRingBufferArraySize);
        pMem += stagingRingBufferArraySize;

        // Map pipeline array
        backendContext->pPipelineLayouts = (BackendContext_VK::PipelineLayout*)pMem;
        memset(backendContext->pPipelineLayouts, 0, pipelineArraySize);
        pMem += pipelineArraySize;

        // Map resource array
        backendContext->pResources = (BackendContext_VK::Resource*)pMem;
        memset(backendContext->pResources, 0, resourceArraySize);
        pMem += resourceArraySize;

        // Clear out all resource mappings
        for (uint32_t i = 0; i < backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT; ++i) {
            backendContext->pResources[i].uavViewIndex = backendContext->pResources[i].srvViewIndex = -1;
        }

        // Map context array. EffectContext is alignas(32), so aligning only
        // each section size to uint32_t (as upstream did) is insufficient.
        pMem = reinterpret_cast<uint8_t*>(FFX_ALIGN_UP(
            reinterpret_cast<uintptr_t>(pMem), alignof(BackendContext_VK::EffectContext)));
        backendContext->pEffectContexts = (BackendContext_VK::EffectContext*)pMem;
        memset(backendContext->pEffectContexts, 0, contextArraySize);
        pMem += contextArraySize;

        // Map extension array
        backendContext->extensionProperties = (VkExtensionProperties*)pMem;

        // if vkGetDeviceProcAddr is NULL, use the one from the vulkan header
        if (vkDeviceContext->vkDeviceProcAddr == NULL)
            vkDeviceContext->vkDeviceProcAddr = vkGetDeviceProcAddr;

        if (vkDeviceContext->vkDevice != VK_NULL_HANDLE) {
            backendContext->device = vkDeviceContext->vkDevice;
        }

        if (vkDeviceContext->vkPhysicalDevice != VK_NULL_HANDLE) {
            backendContext->physicalDevice = vkDeviceContext->vkPhysicalDevice;
        }

        // Vulkan does not expose which optional features were enabled on an
        // existing VkDevice. The application must explicitly opt in; absent
        // metadata is deliberately treated as disabled.
        backendContext->deviceCoherentMemoryEnabled =
            vkDeviceContext->deviceCoherentMemoryEnabled == VK_TRUE ? VK_TRUE : VK_FALSE;
        backendContext->shaderFloat16Enabled =
            vkDeviceContext->shaderFloat16Enabled == VK_TRUE ? VK_TRUE : VK_FALSE;
        backendContext->subgroupSizeControlEnabled =
            vkDeviceContext->subgroupSizeControlEnabled == VK_TRUE ? VK_TRUE : VK_FALSE;
        backendContext->computeFullSubgroupsEnabled =
            vkDeviceContext->computeFullSubgroupsEnabled == VK_TRUE ? VK_TRUE : VK_FALSE;
        backendContext->synchronization2Enabled =
            vkDeviceContext->synchronization2Enabled == VK_TRUE ? VK_TRUE : VK_FALSE;
        backendContext->bufferMarkerEnabled =
            vkDeviceContext->bufferMarkerEnabled == VK_TRUE ? VK_TRUE : VK_FALSE;
        backendContext->debugUtilsEnabled =
            vkDeviceContext->debugUtilsEnabled == VK_TRUE ? VK_TRUE : VK_FALSE;
        backendContext->shaderStorageBufferArrayNonUniformIndexingEnabled =
            vkDeviceContext->shaderStorageBufferArrayNonUniformIndexingEnabled == VK_TRUE ? VK_TRUE : VK_FALSE;
        backendContext->accelerationStructureEnabled =
            vkDeviceContext->accelerationStructureEnabled == VK_TRUE ? VK_TRUE : VK_FALSE;

        // load vulkan functions
        if (backendContext->debugUtilsEnabled)
            backendContext->vkFunctionTable.vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkSetDebugUtilsObjectNameEXT");
        backendContext->vkFunctionTable.vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkFlushMappedMemoryRanges");
        backendContext->vkFunctionTable.vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateDescriptorPool");
        backendContext->vkFunctionTable.vkCreateSampler = (PFN_vkCreateSampler)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateSampler");
        backendContext->vkFunctionTable.vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateDescriptorSetLayout");
        backendContext->vkFunctionTable.vkCreateBuffer = (PFN_vkCreateBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateBuffer");
        backendContext->vkFunctionTable.vkCreateBufferView = (PFN_vkCreateBufferView)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateBufferView");
        backendContext->vkFunctionTable.vkCreateImage = (PFN_vkCreateImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateImage");
        backendContext->vkFunctionTable.vkCreateImageView = (PFN_vkCreateImageView)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateImageView");
        backendContext->vkFunctionTable.vkCreateShaderModule = (PFN_vkCreateShaderModule)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateShaderModule");
        backendContext->vkFunctionTable.vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreatePipelineLayout");
        backendContext->vkFunctionTable.vkCreateComputePipelines = (PFN_vkCreateComputePipelines)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCreateComputePipelines");
        backendContext->vkFunctionTable.vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyPipelineLayout");
        backendContext->vkFunctionTable.vkDestroyPipeline = (PFN_vkDestroyPipeline)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyPipeline");
        backendContext->vkFunctionTable.vkDestroyImage = (PFN_vkDestroyImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyImage");
        backendContext->vkFunctionTable.vkDestroyImageView = (PFN_vkDestroyImageView)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyImageView");
        backendContext->vkFunctionTable.vkDestroyBuffer = (PFN_vkDestroyBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyBuffer");
        backendContext->vkFunctionTable.vkDestroyBufferView = (PFN_vkDestroyBufferView)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyBufferView");
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyDescriptorSetLayout");
        backendContext->vkFunctionTable.vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyDescriptorPool");
        backendContext->vkFunctionTable.vkDestroySampler = (PFN_vkDestroySampler)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroySampler");
        backendContext->vkFunctionTable.vkDestroyShaderModule = (PFN_vkDestroyShaderModule)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkDestroyShaderModule");
        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkGetBufferMemoryRequirements");
        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements2 =
            (PFN_vkGetBufferMemoryRequirements2)vkDeviceContext->vkDeviceProcAddr(
                backendContext->device, "vkGetBufferMemoryRequirements2KHR");
        if (!backendContext->vkFunctionTable.vkGetBufferMemoryRequirements2)
        {
            // VK_KHR_get_memory_requirements2 was promoted in Vulkan 1.1. A
            // Vulkan 1.1+ application need not enable the extension, in which
            // case loaders correctly expose only the unsuffixed core command.
            backendContext->vkFunctionTable.vkGetBufferMemoryRequirements2 =
                (PFN_vkGetBufferMemoryRequirements2)vkDeviceContext->vkDeviceProcAddr(
                    backendContext->device, "vkGetBufferMemoryRequirements2");
        }
        backendContext->vkFunctionTable.vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkGetImageMemoryRequirements");
        backendContext->vkFunctionTable.vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkAllocateDescriptorSets");
        backendContext->vkFunctionTable.vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkFreeDescriptorSets");
        backendContext->vkFunctionTable.vkAllocateMemory = (PFN_vkAllocateMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkAllocateMemory");
        backendContext->vkFunctionTable.vkFreeMemory = (PFN_vkFreeMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkFreeMemory");
        backendContext->vkFunctionTable.vkMapMemory = (PFN_vkMapMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkMapMemory");
        backendContext->vkFunctionTable.vkUnmapMemory = (PFN_vkUnmapMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkUnmapMemory");
        backendContext->vkFunctionTable.vkBindBufferMemory = (PFN_vkBindBufferMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkBindBufferMemory");
        backendContext->vkFunctionTable.vkBindImageMemory = (PFN_vkBindImageMemory)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkBindImageMemory");
        backendContext->vkFunctionTable.vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkUpdateDescriptorSets");
        backendContext->vkFunctionTable.vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdPipelineBarrier");
        backendContext->vkFunctionTable.vkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdBindPipeline");
        backendContext->vkFunctionTable.vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdBindDescriptorSets");
        backendContext->vkFunctionTable.vkCmdDispatch = (PFN_vkCmdDispatch)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdDispatch");
        backendContext->vkFunctionTable.vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdDispatchIndirect");
        backendContext->vkFunctionTable.vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdCopyBuffer");
        backendContext->vkFunctionTable.vkCmdCopyImage = (PFN_vkCmdCopyImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdCopyImage");
        backendContext->vkFunctionTable.vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdCopyBufferToImage");
        backendContext->vkFunctionTable.vkCmdClearColorImage = (PFN_vkCmdClearColorImage)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdClearColorImage");
        backendContext->vkFunctionTable.vkCmdFillBuffer = (PFN_vkCmdFillBuffer)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdFillBuffer");
        if (backendContext->bufferMarkerEnabled)
            backendContext->vkFunctionTable.vkCmdWriteBufferMarkerAMD = (PFN_vkCmdWriteBufferMarkerAMD)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdWriteBufferMarkerAMD");
        if (backendContext->bufferMarkerEnabled && backendContext->synchronization2Enabled)
            backendContext->vkFunctionTable.vkCmdWriteBufferMarker2AMD = (PFN_vkCmdWriteBufferMarker2AMD)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdWriteBufferMarker2AMD");
        if (backendContext->debugUtilsEnabled)
        {
            backendContext->vkFunctionTable.vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdBeginDebugUtilsLabelEXT");
