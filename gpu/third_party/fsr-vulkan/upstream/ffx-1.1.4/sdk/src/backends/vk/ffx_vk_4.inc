        backendContext->vkFunctionTable.vkDestroyDescriptorPool(backendContext->device, backendContext->descriptorPool, VK_NULL_HANDLE);
        backendContext->descriptorPool = VK_NULL_HANDLE;

        // clean up dynamic uniform buffer & memory
        backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, backendContext->uniformBufferMemory);
        backendContext->vkFunctionTable.vkFreeMemory(backendContext->device, backendContext->uniformBufferMemory, VK_NULL_HANDLE);
        backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, backendContext->uniformBuffer, VK_NULL_HANDLE);

        backendContext->device = VK_NULL_HANDLE;
        backendContext->physicalDevice = VK_NULL_HANDLE;

        resetBackendContext(backendContext);
    }

    return FFX_OK;
}

// create a internal resource that will stay alive until effect gets shut down
FfxErrorCode CreateResourceVK(
    FfxInterface* backendInterface,
    const FfxCreateResourceDescription* createResourceDescription,
    FfxUInt32 effectContextId,
    FfxResourceInternal* outResource)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != createResourceDescription);
    FFX_ASSERT(NULL != outResource);
    FFX_ASSERT_MESSAGE(createResourceDescription->initData.type != FFX_RESOURCE_INIT_DATA_TYPE_INVALID,
                       "InitData type cannot be FFX_RESOURCE_INIT_DATA_TYPE_INVALID. Please explicitly specify the resource initialization type.");


    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    VkDevice vkDevice = backendContext->device;

    FFX_ASSERT(VK_NULL_HANDLE != vkDevice);

    VkMemoryPropertyFlags requiredMemoryProperties;

    switch (createResourceDescription->heapType)
    {
    case FFX_HEAP_TYPE_DEFAULT:
        requiredMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    case FFX_HEAP_TYPE_UPLOAD:
    case FFX_HEAP_TYPE_READBACK:
        requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        break;
    default:
        requiredMemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    }

    // Setup the resource description
    FfxResourceDescription resourceDesc = createResourceDescription->resourceDescription;

    if (resourceDesc.mipCount == 0) {
        resourceDesc.mipCount = (uint32_t)(1 + floor(log2(FFX_MAXIMUM(FFX_MAXIMUM(createResourceDescription->resourceDescription.width,
            createResourceDescription->resourceDescription.height), createResourceDescription->resourceDescription.depth))));
    }

    FFX_ASSERT(effectContext.nextStaticResource + 1 < effectContext.nextDynamicResource);
    outResource->internalIndex = effectContext.nextStaticResource++;
    BackendContext_VK::Resource* backendResource = &backendContext->pResources[outResource->internalIndex];
    backendResource->undefined = true;  // A flag to make sure the first barrier for this image resource always uses an src layout of undefined
    backendResource->dynamic = false;   // Not a dynamic resource (need to track them separately for image views)
    backendResource->resourceDescription = resourceDesc;
    backendResource->allocationSize = 0;

    const auto& initData = createResourceDescription->initData;

    const FfxResourceStates resourceState =
        ((initData.type != FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED) && (createResourceDescription->heapType != FFX_HEAP_TYPE_UPLOAD))
            ? FFX_RESOURCE_STATE_COPY_DEST
            : createResourceDescription->initialState;
    backendResource->initialState = resourceState;
    backendResource->currentState = resourceState;

#ifdef _DEBUG
    size_t retval = 0;
    wcstombs_s(&retval, backendResource->resourceName, sizeof(backendResource->resourceName), createResourceDescription->name, sizeof(backendResource->resourceName));
    if (retval >= 64) backendResource->resourceName[63] = '\0';
#endif

    VkMemoryRequirements memRequirements = {};

    switch (createResourceDescription->resourceDescription.type)
    {
    case FFX_RESOURCE_TYPE_BUFFER:
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = createResourceDescription->resourceDescription.width;
        bufferInfo.usage = ffxGetVKBufferUsageFlagsFromResourceUsage(resourceDesc.usage);
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (initData.type != FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (resourceState == FFX_RESOURCE_STATE_COPY_SRC)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        if (resourceState == FFX_RESOURCE_STATE_COPY_DEST)
            bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        if (backendContext->vkFunctionTable.vkCreateBuffer(backendContext->device, &bufferInfo, NULL, &backendResource->bufferResource) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)backendResource->bufferResource, backendResource->resourceName);
#endif

        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, backendResource->bufferResource, &memRequirements);

        // allocate the memory
        FfxErrorCode errorCode = allocateDeviceMemory(backendContext, memRequirements, requiredMemoryProperties, backendResource);
        if (FFX_OK != errorCode)
            return errorCode;

        if (backendContext->vkFunctionTable.vkBindBufferMemory(backendContext->device, backendResource->bufferResource, backendResource->deviceMemory, 0) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

        // if this is an upload buffer (currently only support upload buffers), copy the data and return
        if (createResourceDescription->heapType == FFX_HEAP_TYPE_UPLOAD && initData.size > 0)
        {
            // only allow copies directly into mapped memory for buffer resources since all texture resources are in optimal tiling
            void* data = NULL;

            if (backendContext->vkFunctionTable.vkMapMemory(backendContext->device, backendResource->deviceMemory, 0, initData.size, 0, &data) != VK_SUCCESS) {
                return FFX_ERROR_BACKEND_API_ERROR;
            }

            if (initData.type == FFX_RESOURCE_INIT_DATA_TYPE_BUFFER)
                memcpy(data, initData.buffer, initData.size);
            else if (initData.type == FFX_RESOURCE_INIT_DATA_TYPE_VALUE)
                memset(data, initData.value, initData.size);

            // flush mapped range if memory type is not coherant
            if ((backendResource->memoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange memoryRange = {};
                memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                memoryRange.memory = backendResource->deviceMemory;
                memoryRange.size = initData.size;

                backendContext->vkFunctionTable.vkFlushMappedMemoryRanges(backendContext->device, 1, &memoryRange);
            }

            backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, backendResource->deviceMemory);
            return FFX_OK;
        }

        break;
    }
    case FFX_RESOURCE_TYPE_TEXTURE1D:
    case FFX_RESOURCE_TYPE_TEXTURE2D:
    case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_RESOURCE_TYPE_TEXTURE3D:
    {
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = ffxGetVKImageTypeFromResourceType(createResourceDescription->resourceDescription.type);
        imageInfo.extent.width = createResourceDescription->resourceDescription.width;
        imageInfo.extent.height = createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE1D ? 1 : createResourceDescription->resourceDescription.height;
        imageInfo.extent.depth = ( createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE3D || createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE_CUBE ) ?
            createResourceDescription->resourceDescription.depth : 1;
        imageInfo.mipLevels = backendResource->resourceDescription.mipCount;
        imageInfo.arrayLayers = (createResourceDescription->resourceDescription.type ==
                                FFX_RESOURCE_TYPE_TEXTURE1D || createResourceDescription->resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE2D)
            ? createResourceDescription->resourceDescription.depth : 1;
        imageInfo.format = getVkFormatFromSurfaceFormatAndUsage(createResourceDescription->resourceDescription.format, resourceDesc.usage);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = getVKImageUsageFlagsFromResourceUsage(resourceDesc.usage);
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (FFX_CONTAINS_FLAG(resourceDesc.usage, FFX_RESOURCE_USAGE_UAV) && ffxIsSurfaceFormatSRGB(createResourceDescription->resourceDescription.format))
        {
            imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            imageInfo.format = ffxGetVkFormatFromSurfaceFormat(ffxGetSurfaceFormatFromGamma(createResourceDescription->resourceDescription.format));
        }

        if (backendContext->vkFunctionTable.vkCreateImage(backendContext->device, &imageInfo, nullptr, &backendResource->imageResource) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)backendResource->imageResource, backendResource->resourceName);
#endif

        backendContext->vkFunctionTable.vkGetImageMemoryRequirements(backendContext->device, backendResource->imageResource, &memRequirements);

        // allocate the memory
        FfxErrorCode errorCode = allocateDeviceMemory(backendContext, memRequirements, requiredMemoryProperties, backendResource);
        if (FFX_OK != errorCode)
            return errorCode;

        if (backendContext->vkFunctionTable.vkBindImageMemory(backendContext->device, backendResource->imageResource, backendResource->deviceMemory, 0) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

        break;
    }
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Unsupported resource type creation requested.");
        break;
    }

    // Create SRVs and UAVs
    switch (createResourceDescription->resourceDescription.type)
    {
    case FFX_RESOURCE_TYPE_BUFFER:
        break;
    case FFX_RESOURCE_TYPE_TEXTURE1D:
    case FFX_RESOURCE_TYPE_TEXTURE2D:
    case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_RESOURCE_TYPE_TEXTURE3D:
    {
        FFX_ASSERT_MESSAGE(effectContext.nextStaticResourceView + 1 < effectContext.nextDynamicResourceView[0],
            "FFXInterface: Vulkan: We've run out of resource views. Please increase the size.");
        backendResource->srvViewIndex = effectContext.nextStaticResourceView++;

        FfxResourceType type = createResourceDescription->resourceDescription.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;

        bool requestArrayView = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFX_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFX_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

        imageViewCreateInfo.image = backendResource->imageResource;
        imageViewCreateInfo.format = getVkFormatFromSurfaceFormatAndUsage(createResourceDescription->resourceDescription.format, backendResource->resourceDescription.usage);
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = getImageAspect(backendResource->resourceDescription.usage);
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = backendResource->resourceDescription.mipCount;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VkImageViewUsageCreateInfo imageViewUsageCreateInfo = {};
        addMutableViewForSRV(imageViewCreateInfo, imageViewUsageCreateInfo, backendResource->resourceDescription);

        // create an image view containing all mip levels for use as an srv
        if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->srvViewIndex].imageView) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }
#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->srvViewIndex].imageView, backendResource->resourceName);
#endif

        // create image views of individual mip levels for use as a uav
        if (FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_RESOURCE_USAGE_UAV))
        {
            const int32_t uavResourceViewCount = backendResource->resourceDescription.mipCount;
            FFX_ASSERT(effectContext.nextStaticResourceView + uavResourceViewCount < effectContext.nextDynamicResourceView[0]);

            backendResource->uavViewIndex = effectContext.nextStaticResourceView;
            backendResource->uavViewCount = uavResourceViewCount;

            imageViewCreateInfo.format = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_RESOURCE_USAGE_DEPTHTARGET) ? VK_FORMAT_D32_SFLOAT : ffxGetVKUAVFormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);

            for (uint32_t mip = 0; mip < backendResource->resourceDescription.mipCount; ++mip)
            {
                imageViewCreateInfo.subresourceRange.levelCount = 1;
                imageViewCreateInfo.subresourceRange.baseMipLevel = mip;

                if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView) != VK_SUCCESS) {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
#ifdef _DEBUG
                setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView, backendResource->resourceName);
#endif
            }

            effectContext.nextStaticResourceView += uavResourceViewCount;
        }
        break;
    }
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Unsupported resource view type creation requested.");
        break;
    }

    // create upload resource and upload job if needed
    if (initData.type != FFX_RESOURCE_INIT_DATA_TYPE_UNINITIALIZED)
    {
        FfxResourceInternal copySrc;
        FfxCreateResourceDescription uploadDesc = { *createResourceDescription };
        uploadDesc.heapType = FFX_HEAP_TYPE_UPLOAD;
        uploadDesc.resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
        uploadDesc.resourceDescription.width = static_cast<uint32_t>(initData.size);
        uploadDesc.resourceDescription.usage = FFX_RESOURCE_USAGE_READ_ONLY;
        uploadDesc.initialState = FFX_RESOURCE_STATE_GENERIC_READ;
        uploadDesc.initData = createResourceDescription->initData;

        backendInterface->fpCreateResource(backendInterface, &uploadDesc, effectContextId, &copySrc);

        // setup the upload job
        FfxGpuJobDescription copyJob  = { FFX_GPU_JOB_COPY, L"Resource Initialization Copy" };
        copyJob.copyJobDescriptor.src = copySrc;
        copyJob.copyJobDescriptor.dst = *outResource;
        copyJob.copyJobDescriptor.srcOffset = 0;
        copyJob.copyJobDescriptor.dstOffset = 0;
        copyJob.copyJobDescriptor.size      = 0;

        backendInterface->fpScheduleGpuJob(backendInterface, &copyJob);
    }

    backendResource->allocationSize = memRequirements.size;
    effectContext.vramUsage.totalUsageInBytes += static_cast<uint64_t>(backendResource->allocationSize);
    if ((createResourceDescription->resourceDescription.flags & FFX_RESOURCE_FLAGS_ALIASABLE) == FFX_RESOURCE_FLAGS_ALIASABLE)
    {
        effectContext.vramUsage.aliasableUsageInBytes += static_cast<uint64_t>(backendResource->allocationSize);
    }

    return FFX_OK;
}

FfxErrorCode DestroyResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId)
{
    FFX_ASSERT(backendInterface != nullptr);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    if ((resource.internalIndex >= int32_t(effectContextId * FFX_MAX_RESOURCE_COUNT)) && (resource.internalIndex < int32_t(effectContext.nextStaticResource)))
    {
        BackendContext_VK::Resource& backgroundResource = backendContext->pResources[resource.internalIndex];

        if (backgroundResource.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER)
        {
            // Destroy the resource
            if (backgroundResource.bufferResource != VK_NULL_HANDLE)
            {
                backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, backgroundResource.bufferResource, nullptr);
                backgroundResource.bufferResource = VK_NULL_HANDLE;
            }
        }
        else
        {
            // Destroy SRV
            if (backgroundResource.srvViewIndex >= 0)
            {
                backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[backgroundResource.srvViewIndex].imageView, nullptr);
                backendContext->pResourceViews[backgroundResource.srvViewIndex].imageView = VK_NULL_HANDLE;
                backgroundResource.srvViewIndex                                           = 0;
            }

             // And UAVs
             if (FFX_CONTAINS_FLAG(backgroundResource.resourceDescription.usage, FFX_RESOURCE_USAGE_UAV))
             {
                 for (uint32_t i = 0; i < backgroundResource.uavViewCount; ++i)
                 {
                     if (backendContext->pResourceViews[backgroundResource.uavViewIndex + i].imageView != VK_NULL_HANDLE)
                     {
                         backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[backgroundResource.uavViewIndex + i].imageView, nullptr);
                         backendContext->pResourceViews[backgroundResource.uavViewIndex + i].imageView = VK_NULL_HANDLE;
                     }
                 }
             }

            // Reset indices to resource views
            backgroundResource.uavViewIndex = backgroundResource.srvViewIndex = -1;
            backgroundResource.uavViewCount = 0;

            // Destroy the resource
            if (backgroundResource.imageResource != VK_NULL_HANDLE)
            {
                backendContext->vkFunctionTable.vkDestroyImage(backendContext->device, backgroundResource.imageResource, nullptr);
                backgroundResource.imageResource = VK_NULL_HANDLE;
            }
        }

        if (backgroundResource.deviceMemory)
        {
            backendContext->vkFunctionTable.vkFreeMemory(backendContext->device, backgroundResource.deviceMemory, nullptr);
            backgroundResource.deviceMemory = VK_NULL_HANDLE;

            effectContext.vramUsage.totalUsageInBytes -= static_cast<uint64_t>(backgroundResource.allocationSize);
            if ((backendContext->pResources[resource.internalIndex].resourceDescription.flags & FFX_RESOURCE_FLAGS_ALIASABLE) == FFX_RESOURCE_FLAGS_ALIASABLE)
            {
                effectContext.vramUsage.aliasableUsageInBytes -= static_cast<uint64_t>(backgroundResource.allocationSize);
            }
            backgroundResource.allocationSize = 0;
        }
    }

    return FFX_OK;
}

FfxErrorCode MapResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource, void** ptr)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    BackendContext_VK::Resource* res = &backendContext->pResources[resource.internalIndex];

    if ((backendContext->vkFunctionTable.vkMapMemory(backendContext->device, res->deviceMemory, 0, res->resourceDescription.size, 0, ptr) != VK_SUCCESS))
        return FFX_ERROR_BACKEND_API_ERROR;

    return FFX_OK;
}

FfxErrorCode UnmapResourceVK(FfxInterface* backendInterface, FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    BackendContext_VK::Resource* res = &backendContext->pResources[resource.internalIndex];

    backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, res->deviceMemory);

    return FFX_OK;
}

FfxErrorCode RegisterResourceVK(
    FfxInterface* backendInterface,
    const FfxResource* inFfxResource,
    FfxUInt32 effectContextId,
    FfxResourceInternal* outFfxResourceInternal
)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)(backendInterface->scratchBuffer);
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    if (inFfxResource->resource == nullptr) {

        outFfxResourceInternal->internalIndex = 0; // Always maps to FFX_<feature>_RESOURCE_IDENTIFIER_NULL;
        return FFX_OK;
    }

    // In vulkan we need to treat dynamic resources a little differently due to needing views to live as long as the GPU needs them.
    // We will treat them more like static resources and use the nextDynamicResource as a "hint" for where it should be.
    // Failure to find the pre-existing resource at the expected location will force a search until the resource is found.
    // If it is not found, a new entry will be created
    FFX_ASSERT(effectContext.nextDynamicResource > effectContext.nextStaticResource);
    outFfxResourceInternal->internalIndex = effectContext.nextDynamicResource--;

    //bool setupDynamicResource = false;
    BackendContext_VK::Resource* backendResource = &backendContext->pResources[outFfxResourceInternal->internalIndex];

    // Start by seeing if this entry is empty, as that triggers an automatic setup of a new dynamic resource
    /*if (backendResource->uavViewIndex < 0 && backendResource->srvViewIndex < 0)
    {
        setupDynamicResource = true;
    }

    // If not a new resource, does it match what's current slotted for this dynamic resource
    if (!setupDynamicResource)
    {
        // If this is us, just return as everything is setup as needed
        if ((backendResource->resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER && backendResource->bufferResource == (VkBuffer)inFfxResource->resource) ||
            (backendResource->resourceDescription.type != FFX_RESOURCE_TYPE_BUFFER && backendResource->imageResource == (VkImage)inFfxResource->resource))
            return FFX_OK;

        // If this isn't us, search until we either find our entry or an empty resource
        outFfxResourceInternal->internalIndex = (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
        while (!setupDynamicResource)
        {
            FFX_ASSERT(outFfxResourceInternal->internalIndex > effectContext.nextStaticResource); // Safety check while iterating
            backendResource = &backendContext->pResources[outFfxResourceInternal->internalIndex];

            // Is this us?
            if ((backendResource->resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER && backendResource->bufferResource == (VkBuffer)inFfxResource->resource) ||
                (backendResource->resourceDescription.type != FFX_RESOURCE_TYPE_BUFFER && backendResource->imageResource == (VkImage)inFfxResource->resource))
            {
                 copyResourceState(backendResource, inFfxResource);
                 return FFX_OK;
            }

            // Empty?
            if (backendResource->uavViewIndex == -1 && backendResource->srvViewIndex == -1)
            {
                setupDynamicResource = true;
                break;
            }

            --outFfxResourceInternal->internalIndex;
        }
    }*/

    // If we got here, we are setting up a new dynamic entry
    backendResource->resourceDescription = inFfxResource->description;
    if (inFfxResource->description.type == FFX_RESOURCE_TYPE_BUFFER)
        backendResource->bufferResource = reinterpret_cast<VkBuffer>(inFfxResource->resource);
    else
        backendResource->imageResource = reinterpret_cast<VkImage>(inFfxResource->resource);

    copyResourceState(backendResource, inFfxResource);

#ifdef _DEBUG
    size_t retval = 0;
    wcstombs_s(&retval, backendResource->resourceName, sizeof(backendResource->resourceName), inFfxResource->name, sizeof(backendResource->resourceName));
    if (retval >= 64) backendResource->resourceName[63] = '\0';
#endif

    // the first call of RegisterResource can be identified because


    //////////////////////////////////////////////////////////////////////////
    // Create SRVs and UAVs
    switch (backendResource->resourceDescription.type)
    {
    case FFX_RESOURCE_TYPE_BUFFER:
        break;
    case FFX_RESOURCE_TYPE_TEXTURE1D:
    case FFX_RESOURCE_TYPE_TEXTURE2D:
    case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_RESOURCE_TYPE_TEXTURE3D:
    {
        FfxResourceType type = backendResource->resourceDescription.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;

        bool requestArrayView = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFX_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFX_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (backendResource->resourceDescription.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

        imageViewCreateInfo.image = backendResource->imageResource;
        imageViewCreateInfo.format = getVkFormatFromSurfaceFormatAndUsage(backendResource->resourceDescription.format, backendResource->resourceDescription.usage);
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = getImageAspect(backendResource->resourceDescription.usage);
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = backendResource->resourceDescription.mipCount;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        // create an image view containing all mip levels for use as an srv
        FFX_ASSERT(effectContext.nextDynamicResourceView[effectContext.frameIndex] > ((effectContext.frameIndex == 0) ? effectContext.nextStaticResourceView : getDynamicResourceViewsStartIndex(effectContextId, effectContext.frameIndex - 1)));
        backendResource->srvViewIndex = effectContext.nextDynamicResourceView[effectContext.frameIndex]--;

        VkImageViewUsageCreateInfo imageViewUsageCreateInfo = {};
        addMutableViewForSRV(imageViewCreateInfo, imageViewUsageCreateInfo, backendResource->resourceDescription);

        if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->srvViewIndex].imageView) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }
#ifdef _DEBUG
        setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->srvViewIndex].imageView, backendResource->resourceName);
#endif

        // create image views of individual mip levels for use as a uav
        if (FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_RESOURCE_USAGE_UAV))
        {
            const int32_t uavResourceViewCount = backendResource->resourceDescription.mipCount;
            FFX_ASSERT(effectContext.nextDynamicResourceView[effectContext.frameIndex] - uavResourceViewCount + 1 > ((effectContext.frameIndex == 0) ? effectContext.nextStaticResourceView : getDynamicResourceViewsStartIndex(effectContextId, effectContext.frameIndex - 1)));
            backendResource->uavViewIndex = effectContext.nextDynamicResourceView[effectContext.frameIndex] - uavResourceViewCount + 1;
            backendResource->uavViewCount = uavResourceViewCount;

            imageViewCreateInfo.format = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_RESOURCE_USAGE_DEPTHTARGET) ? VK_FORMAT_D32_SFLOAT : ffxGetVKUAVFormatFromSurfaceFormat(backendResource->resourceDescription.format);
            imageViewCreateInfo.pNext  = nullptr;

            for (uint32_t mip = 0; mip < backendResource->resourceDescription.mipCount; ++mip)
            {
                imageViewCreateInfo.subresourceRange.levelCount = 1;
                imageViewCreateInfo.subresourceRange.baseMipLevel = mip;

                if (backendContext->vkFunctionTable.vkCreateImageView(backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView) != VK_SUCCESS) {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
#ifdef _DEBUG
                setVKObjectName(backendContext->vkFunctionTable, backendContext->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)backendContext->pResourceViews[backendResource->uavViewIndex + mip].imageView, backendResource->resourceName);
#endif
            }
            effectContext.nextDynamicResourceView[effectContext.frameIndex] -= uavResourceViewCount;
        }
        break;
    }
    default:
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Unsupported resource view type creation requested.");
        break;
    }

    return FFX_OK;
}

FfxResource GetResourceVK(FfxInterface* backendInterface, FfxResourceInternal inResource)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FfxResourceDescription ffxResDescription = backendInterface->fpGetResourceDescription(backendInterface, inResource);

    FfxResource resource = {};
    resource.resource = reinterpret_cast<void*>(backendContext->pResources[inResource.internalIndex].imageResource);
    // If the internal resource state is undefined, that means we are importing a resource that
    // has not yet been initialized, so we will flag it as such to finish initializing it later
    // before it is used.
    if (backendContext->pResources[inResource.internalIndex].undefined) {
        ffxResDescription.flags = (FfxResourceFlags)((int)ffxResDescription.flags | FFX_RESOURCE_FLAGS_UNDEFINED);
        // Flag it as no longer being undefined as it will no longer be after workload
        // execution
        backendContext->pResources[inResource.internalIndex].undefined = false;
    }
    resource.state = backendContext->pResources[inResource.internalIndex].currentState;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (backendContext->pResources[inResource.internalIndex].resourceName)
    {
        ConvertUTF8ToUTF16(backendContext->pResources[inResource.internalIndex].resourceName, resource.name, 64);
    }
#endif

    return resource;
}

// dispose dynamic resources: This should be called at the end of the frame
FfxErrorCode UnregisterResourcesVK(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)(backendInterface->scratchBuffer);
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Walk back all the resources that don't belong to us and reset them to their initial state
    const uint32_t dynamicResourceIndexStart = getDynamicResourcesStartIndex(effectContextId);
    for (uint32_t resourceIndex = ++effectContext.nextDynamicResource; resourceIndex <= dynamicResourceIndexStart; ++resourceIndex)
    {
        FfxResourceInternal internalResource;
        internalResource.internalIndex = resourceIndex;

        BackendContext_VK::Resource* backendResource = &backendContext->pResources[resourceIndex];

        // Also clear out their srv/uav indices so they are regenerated each frame
        backendResource->uavViewIndex = -1;
        backendResource->srvViewIndex = -1;

        // Add the barrier
        addBarrier(backendContext, &internalResource, backendResource->initialState);
    }

    FFX_ASSERT(nullptr != commandList);
    VkCommandBuffer pCmdList = reinterpret_cast<VkCommandBuffer>(commandList);

    flushBarriers(backendContext, pCmdList);

    // Just reset the dynamic resource index, but leave the images views.
    // They will be deleted in the first pipeline destroy call as they need to live until then
    effectContext.nextDynamicResource = dynamicResourceIndexStart;

    // destroy the views of the next frame
    effectContext.frameIndex = (effectContext.frameIndex + 1) % FFX_MAX_QUEUED_FRAMES;
    destroyDynamicViews(backendContext, effectContextId, effectContext.frameIndex);

    return FFX_OK;
}

FfxErrorCode registerStaticTextureSrv(BackendContext_VK* backendContext, const FfxResource* inResource, uint32_t index, FfxUInt32 effectContextId)
{            
    BackendContext_VK::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    if (effectContext.bindlessTextureSrvHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    VkImage vkImage = reinterpret_cast<VkImage>(inResource->resource);

    // Create SRVs and UAVs
    switch (inResource->description.type)
    {
    case FFX_RESOURCE_TYPE_BUFFER:
    {
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Attempting to register a Buffer as a Texture SRV.");
        break;
    }
    case FFX_RESOURCE_TYPE_TEXTURE1D:
    case FFX_RESOURCE_TYPE_TEXTURE2D:
    case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
    case FFX_RESOURCE_TYPE_TEXTURE3D:
    {
        FfxResourceType       type                = inResource->description.type;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext                 = nullptr;

        bool requestArrayView = FFX_CONTAINS_FLAG(inResource->description.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

        switch (type)
        {
        case FFX_RESOURCE_TYPE_TEXTURE1D:
            imageViewCreateInfo.viewType = (inResource->description.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        default:
        case FFX_RESOURCE_TYPE_TEXTURE2D:
            imageViewCreateInfo.viewType = (inResource->description.depth > 1 || requestArrayView) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case FFX_RESOURCE_TYPE_TEXTURE3D:
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        }

