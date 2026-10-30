        imageViewCreateInfo.image                           = vkImage;
        imageViewCreateInfo.format                          = getVkFormatFromSurfaceFormatAndUsage(inResource->description.format, inResource->description.usage);
        imageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask     = getImageAspect(inResource->description.usage);
        imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        imageViewCreateInfo.subresourceRange.levelCount     = inResource->description.mipCount;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

        uint32_t imageViewIndex = effectContext.bindlessTextureSrvHeapStart + index;

        if (backendContext->pResourceViews[imageViewIndex].imageView)
            backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[imageViewIndex].imageView, nullptr);

        if (backendContext->vkFunctionTable.vkCreateImageView(
                backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[imageViewIndex].imageView) != VK_SUCCESS)
        {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

#ifdef _DEBUG
        size_t retval = 0;
        char   resourceName[64];
        wcstombs_s(&retval, resourceName, sizeof(resourceName), inResource->name, sizeof(resourceName));
        if (retval >= 64) resourceName[63] = '\0';

        setVKObjectName(backendContext->vkFunctionTable,
                        backendContext->device,
                        VK_OBJECT_TYPE_IMAGE_VIEW,
                        (uint64_t)backendContext->pResourceViews[imageViewIndex].imageView,
                        resourceName);
#endif

        VkWriteDescriptorSet  writeDescriptorSet  = {};
        VkDescriptorImageInfo imageDescriptorInfo = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};

        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet          = effectContext.bindlessTextureSrvDescriptorSet;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeDescriptorSet.pImageInfo      = &imageDescriptorInfo;
        writeDescriptorSet.dstBinding      = 0;
        writeDescriptorSet.dstArrayElement = index;

        imageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageDescriptorInfo.imageView   = backendContext->pResourceViews[imageViewIndex].imageView;

        backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, 1, &writeDescriptorSet, 0, nullptr);

        return FFX_OK;

        break;
    }
    default:
        break;
    }

    return FFX_ERROR_INVALID_ARGUMENT;
}            
             
FfxErrorCode registerStaticBufferSrv(BackendContext_VK* backendContext, const FfxResource* inResource, uint32_t offset, uint32_t size, uint32_t stride, uint32_t index, FfxUInt32 effectContextId)
{            
    BackendContext_VK::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    if (effectContext.bindlessBufferSrvHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    if (inResource->description.type == FFX_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer buffer = reinterpret_cast<VkBuffer>(inResource->resource);

        VkWriteDescriptorSet   writeDescriptorSet   = {};
        VkDescriptorBufferInfo bufferDescriptorInfo = {VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};

        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet          = effectContext.bindlessBufferSrvDescriptorSet;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSet.pBufferInfo     = &bufferDescriptorInfo;
        writeDescriptorSet.dstBinding      = 0;
        writeDescriptorSet.dstArrayElement = index;

        bufferDescriptorInfo.buffer = buffer;
        bufferDescriptorInfo.offset = offset;
        bufferDescriptorInfo.range  = size > 0 ? size : VK_WHOLE_SIZE;

        backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, 1, &writeDescriptorSet, 0, nullptr);

        return FFX_OK;
    }
    else
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Attempting to register a Texture as a Buffer SRV.");

    return FFX_ERROR_INVALID_ARGUMENT;
}            
             
FfxErrorCode registerStaticTextureUav(BackendContext_VK* backendContext, const FfxResource* inResource, uint32_t mip, uint32_t index, FfxUInt32 effectContextId)
{            
    BackendContext_VK::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    if (effectContext.bindlessTextureUavHeapSize <= index)
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
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Attempting to register a Buffer as a Texture UAV.");
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

        imageViewCreateInfo.image                           = vkImage;
        imageViewCreateInfo.format                          = getVkFormatFromSurfaceFormatAndUsage(inResource->description.format, inResource->description.usage);
        imageViewCreateInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask     = getImageAspect(inResource->description.usage);
        imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        imageViewCreateInfo.subresourceRange.levelCount     = inResource->description.mipCount;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;

        uint32_t imageViewIndex = effectContext.bindlessTextureUavHeapStart + index;

        if (backendContext->pResourceViews[imageViewIndex].imageView)
            backendContext->vkFunctionTable.vkDestroyImageView(backendContext->device, backendContext->pResourceViews[imageViewIndex].imageView, nullptr);

        if (backendContext->vkFunctionTable.vkCreateImageView(
                backendContext->device, &imageViewCreateInfo, NULL, &backendContext->pResourceViews[imageViewIndex].imageView) != VK_SUCCESS)
        {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

#ifdef _DEBUG
        size_t retval = 0;
        char   resourceName[64];
        wcstombs_s(&retval, resourceName, sizeof(resourceName), inResource->name, sizeof(resourceName));
        if (retval >= 64) resourceName[63] = '\0';

        setVKObjectName(backendContext->vkFunctionTable,
                        backendContext->device,
                        VK_OBJECT_TYPE_IMAGE_VIEW,
                        (uint64_t)backendContext->pResourceViews[imageViewIndex].imageView,
                        resourceName);
#endif

        VkWriteDescriptorSet  writeDescriptorSet  = {};
        VkDescriptorImageInfo imageDescriptorInfo = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};

        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet          = effectContext.bindlessTextureUavDescriptorSet;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSet.pImageInfo      = &imageDescriptorInfo;
        writeDescriptorSet.dstBinding      = 0;
        writeDescriptorSet.dstArrayElement = index;

        imageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageDescriptorInfo.imageView   = backendContext->pResourceViews[imageViewIndex].imageView;

        backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, 1, &writeDescriptorSet, 0, nullptr);

        return FFX_OK;

        break;
    }
    default:
        break;
    }

    return FFX_ERROR_INVALID_ARGUMENT;
}            
             
FfxErrorCode registerStaticBufferUav(BackendContext_VK* backendContext, const FfxResource* inResource, uint32_t offset, uint32_t size, uint32_t stride, uint32_t index, FfxUInt32 effectContextId)
{
    BackendContext_VK::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    if (effectContext.bindlessBufferUavHeapSize <= index)
    {
        FFX_ASSERT(false);
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    }

    if (inResource->description.type == FFX_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer buffer = reinterpret_cast<VkBuffer>(inResource->resource);

        VkWriteDescriptorSet   writeDescriptorSet   = {};
        VkDescriptorBufferInfo bufferDescriptorInfo = {VK_NULL_HANDLE, 0, VK_WHOLE_SIZE};

        writeDescriptorSet.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet          = effectContext.bindlessBufferUavDescriptorSet;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSet.pBufferInfo     = &bufferDescriptorInfo;
        writeDescriptorSet.dstBinding      = 0;
        writeDescriptorSet.dstArrayElement = index;

        bufferDescriptorInfo.buffer = buffer;
        bufferDescriptorInfo.offset = offset;
        bufferDescriptorInfo.range  = size > 0 ? size : VK_WHOLE_SIZE;

        backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, 1, &writeDescriptorSet, 0, nullptr);

        return FFX_OK;
    }
    else
        FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Attempting to register a Texture as a Buffer UAV.");

    return FFX_ERROR_INVALID_ARGUMENT;
}

FfxErrorCode RegisterStaticResourceVK(FfxInterface* backendInterface, const FfxStaticResourceDescription* desc, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != desc);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    switch (desc->descriptorType)
    {
    case FFX_DESCRIPTOR_TEXTURE_SRV:
        return registerStaticTextureSrv(backendContext, desc->resource, desc->descriptorIndex, effectContextId);
    case FFX_DESCRIPTOR_BUFFER_SRV:
        return registerStaticBufferSrv(
            backendContext, desc->resource, desc->bufferOffset, desc->bufferSize, desc->bufferStride, desc->descriptorIndex, effectContextId);
    case FFX_DESCRIPTOR_TEXTURE_UAV:
        return registerStaticTextureUav(backendContext, desc->resource, desc->textureUavMip, desc->descriptorIndex, effectContextId);
    case FFX_DESCRIPTOR_BUFFER_UAV:
        return registerStaticBufferUav(
            backendContext, desc->resource, desc->bufferOffset, desc->bufferSize, desc->bufferStride, desc->descriptorIndex, effectContextId);
    default:
        return FFX_ERROR_INVALID_ARGUMENT;
    }
}

FfxResourceDescription GetResourceDescriptionVK(FfxInterface* backendInterface, FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FfxResourceDescription resourceDescription = backendContext->pResources[resource.internalIndex].resourceDescription;
    return resourceDescription;
}

FfxErrorCode StageConstantBufferDataVK(FfxInterface* backendInterface, void* data, FfxUInt32 size, FfxConstantBuffer* constantBuffer)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    if (data && constantBuffer)
    {
        if ((backendContext->stagingRingBufferBase + FFX_ALIGN_UP(size, 256)) >= FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE)
            backendContext->stagingRingBufferBase = 0;

        uint32_t* dstPtr = (uint32_t*)(backendContext->pStagingRingBuffer + backendContext->stagingRingBufferBase);

        memcpy(dstPtr, data, size);

        constantBuffer->data            = dstPtr;
        constantBuffer->num32BitEntries = size / sizeof(uint32_t);

        backendContext->stagingRingBufferBase += FFX_ALIGN_UP(size, 256);

        return FFX_OK;
    }
    else
        return FFX_ERROR_INVALID_POINTER;
}

VkSamplerAddressMode FfxGetAddressModeVK(const FfxAddressMode& addressMode)
{
    switch (addressMode)
    {
    case FFX_ADDRESS_MODE_WRAP:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case FFX_ADDRESS_MODE_MIRROR:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case FFX_ADDRESS_MODE_CLAMP:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case FFX_ADDRESS_MODE_BORDER:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case FFX_ADDRESS_MODE_MIRROR_ONCE:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default:
        FFX_ASSERT_MESSAGE(false, "Unsupported addressing mode requested. Please implement");
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        break;
    }
}

FfxErrorCode CreatePipelineVK(FfxInterface* backendInterface,
    FfxEffect effect,
    FfxPass pass,
    uint32_t permutationOptions,
    const FfxPipelineDescription* pipelineDescription,
    FfxUInt32 effectContextId,
    FfxPipelineState* outPipeline)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != pipelineDescription);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // start by fetching the shader blob
    FfxShaderBlob shaderBlob = { };
    // WON'T WORK WITH FSR3!!
    backendInterface->fpGetPermutationBlobByIndex(effect, pass, FFX_BIND_COMPUTE_SHADER_STAGE, permutationOptions, &shaderBlob);
    FFX_ASSERT(shaderBlob.data && shaderBlob.size);

    //////////////////////////////////////////////////////////////////////////
    // One root signature (or pipeline layout) per pipeline
    FFX_ASSERT_MESSAGE(effectContext.nextPipelineLayout < (effectContextId * FFX_MAX_PASS_COUNT) + FFX_MAX_PASS_COUNT, "FFXInterface: Vulkan: Ran out of pipeline layouts. Please increase FFX_MAX_PASS_COUNT");
    BackendContext_VK::PipelineLayout* pPipelineLayout = &backendContext->pPipelineLayouts[effectContext.nextPipelineLayout++];

    // Start by creating samplers
    FFX_ASSERT(pipelineDescription->samplerCount <= FFX_MAX_SAMPLERS);
    const size_t samplerCount = pipelineDescription->samplerCount;
    for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < samplerCount; ++currentSamplerIndex)
    {
        VkSamplerCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.minLod = 0.f;
        createInfo.maxLod = VK_LOD_CLAMP_NONE;
        createInfo.anisotropyEnable = false; // TODO: Do a check for anisotropy once it's an available filter option
        createInfo.compareEnable = false;
        createInfo.compareOp = VK_COMPARE_OP_NEVER;
        createInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
        createInfo.unnormalizedCoordinates = VK_FALSE;
        createInfo.addressModeU = FfxGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeU);
        createInfo.addressModeV = FfxGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeV);
        createInfo.addressModeW = FfxGetAddressModeVK(pipelineDescription->samplers[currentSamplerIndex].addressModeW);

        // Set the right filter
        switch (pipelineDescription->samplers[currentSamplerIndex].filter)
        {
        case FFX_FILTER_TYPE_MINMAGMIP_POINT:
            createInfo.minFilter = VK_FILTER_NEAREST;
            createInfo.magFilter = VK_FILTER_NEAREST;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case FFX_FILTER_TYPE_MINMAGMIP_LINEAR:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case FFX_FILTER_TYPE_MINMAGLINEARMIP_POINT:
            createInfo.minFilter = VK_FILTER_LINEAR;
            createInfo.magFilter = VK_FILTER_LINEAR;
            createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;

        default:
            FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: Unsupported filter type requested. Please implement");
            break;
        }

        if (backendContext->vkFunctionTable.vkCreateSampler(backendContext->device, &createInfo, nullptr, &pPipelineLayout->samplers[currentSamplerIndex]) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }

    int32_t staticTextureSrvCount = 0;
    int32_t staticBufferSrvCount  = 0;
    int32_t staticTextureUavCount = 0;
    int32_t staticBufferUavCount  = 0;

    pPipelineLayout->staticTextureSrvSet = -1;
    pPipelineLayout->staticBufferSrvSet  = -1;
    pPipelineLayout->staticTextureUavSet = -1;
    pPipelineLayout->staticBufferUavSet  = -1;

    // Setup descriptor sets
    VkDescriptorSetLayoutBinding layoutBindings[MAX_DESCRIPTOR_SET_LAYOUTS];
    uint32_t numLayoutBindings = 0;

    // Support more when needed
    VkShaderStageFlags shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Samplers - just the static ones for now
    for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < samplerCount; ++currentSamplerIndex)
        layoutBindings[numLayoutBindings++] = {
            currentSamplerIndex + SAMPLER_BINDING_SHIFT, VK_DESCRIPTOR_TYPE_SAMPLER, 1, shaderStageFlags, &pPipelineLayout->samplers[currentSamplerIndex]};

    // Texture SRVs
    for (uint32_t srvIndex = 0; srvIndex < shaderBlob.srvTextureCount; ++srvIndex)
    {
        // count static srvs separately.
        if (shaderBlob.boundSRVTextureSpaces[srvIndex] != 0)
        {
            if (staticTextureSrvCount > 0)
                FFX_ASSERT(pPipelineLayout->staticTextureSrvSet != shaderBlob.boundSRVTextureSpaces[srvIndex]);

            staticTextureSrvCount += shaderBlob.boundSRVTextureCounts[srvIndex];
            pPipelineLayout->staticTextureSrvSet = shaderBlob.boundSRVTextureSpaces[srvIndex];
            continue;
        }

        layoutBindings[numLayoutBindings++] = { shaderBlob.boundSRVTextures[srvIndex], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            shaderBlob.boundSRVTextureCounts[srvIndex], shaderStageFlags, nullptr };
    }

    // Buffer SRVs
    for (uint32_t srvIndex = 0; srvIndex < shaderBlob.srvBufferCount; ++srvIndex)
    {
        // count static srvs separately.
        if (shaderBlob.boundSRVBufferSpaces[srvIndex] != 0)
        {
            if (staticBufferSrvCount > 0)
                FFX_ASSERT(pPipelineLayout->staticBufferSrvSet != shaderBlob.boundSRVBufferSpaces[srvIndex]);

            staticBufferSrvCount += shaderBlob.boundSRVBufferCounts[srvIndex];
            pPipelineLayout->staticBufferSrvSet = shaderBlob.boundSRVBufferSpaces[srvIndex];
            continue;
        }

        layoutBindings[numLayoutBindings++] = { shaderBlob.boundSRVBuffers[srvIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            shaderBlob.boundSRVBufferCounts[srvIndex], shaderStageFlags, nullptr};
    }

    // Texture UAVs
    for (uint32_t uavIndex = 0; uavIndex < shaderBlob.uavTextureCount; ++uavIndex)
    {
        // count static uavs separately.
        if (shaderBlob.boundUAVTextureSpaces[uavIndex] != 0)
        {
            if (staticTextureUavCount > 0)
                FFX_ASSERT(pPipelineLayout->staticTextureUavSet != shaderBlob.boundUAVTextureSpaces[uavIndex]);

            staticTextureUavCount += shaderBlob.boundUAVTextureCounts[uavIndex];
            pPipelineLayout->staticTextureUavSet = shaderBlob.boundUAVTextureSpaces[uavIndex];
            continue;
        }

        layoutBindings[numLayoutBindings++] = { shaderBlob.boundUAVTextures[uavIndex], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            shaderBlob.boundUAVTextureCounts[uavIndex], shaderStageFlags, nullptr };
    }

    // Buffer UAVs
    for (uint32_t uavIndex = 0; uavIndex < shaderBlob.uavBufferCount; ++uavIndex)
    {
        // count static uavs separately.
        if (shaderBlob.boundUAVBufferSpaces[uavIndex] != 0)
        {
            if (staticBufferUavCount > 0)
                FFX_ASSERT(pPipelineLayout->staticBufferUavSet != shaderBlob.boundUAVBufferSpaces[uavIndex]);

            staticBufferUavCount += shaderBlob.boundUAVBufferCounts[uavIndex];
            pPipelineLayout->staticBufferUavSet = shaderBlob.boundUAVBufferSpaces[uavIndex];
            continue;
        }

        layoutBindings[numLayoutBindings++] = { shaderBlob.boundUAVBuffers[uavIndex], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            shaderBlob.boundUAVBufferCounts[uavIndex], shaderStageFlags, nullptr };
    }

    // Constant buffers (uniforms)
    for (uint32_t cbIndex = 0; cbIndex < shaderBlob.cbvCount; ++cbIndex)
    {
        layoutBindings[numLayoutBindings++] = { shaderBlob.boundConstantBuffers[cbIndex], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            shaderBlob.boundConstantBufferCounts[cbIndex], shaderStageFlags, nullptr };
    }

    // Create the descriptor layout
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = numLayoutBindings;
    layoutInfo.pBindings = layoutBindings;

    if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(backendContext->device, &layoutInfo, nullptr, &pPipelineLayout->descriptorSetLayout) != VK_SUCCESS) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    // allocate descriptor sets
    pPipelineLayout->descriptorSetIndex = 0;
    for (uint32_t i = 0; i < (FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME); i++)
    {
        VkDescriptorSetAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = backendContext->descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &pPipelineLayout->descriptorSetLayout;

        if (backendContext->vkFunctionTable.vkAllocateDescriptorSets(backendContext->device, &allocateInfo, &pPipelineLayout->descriptorSets[i]) != VK_SUCCESS)
        {
            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }

    uint32_t setCount = 0;

    VkDescriptorSetLayout layouts[5];

    layouts[setCount++] = pPipelineLayout->descriptorSetLayout;

    if (staticTextureSrvCount > 0)
    {
        layouts[pPipelineLayout->staticTextureSrvSet] = effectContext.bindlessTextureSrvDescriptorSetLayout;
        setCount++;
    }

    if (staticBufferSrvCount > 0)
    {
        layouts[pPipelineLayout->staticBufferSrvSet] = effectContext.bindlessBufferSrvDescriptorSetLayout;
        setCount++;
    }

    if (staticTextureUavCount > 0)
    {
        layouts[pPipelineLayout->staticTextureUavSet] = effectContext.bindlessTextureUavDescriptorSetLayout;
        setCount++;
    }

    if (staticBufferUavCount > 0)
    {
        layouts[pPipelineLayout->staticBufferUavSet] = effectContext.bindlessBufferUavDescriptorSetLayout;
        setCount++;
    }

    // create the pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = setCount;
    pipelineLayoutInfo.pSetLayouts            = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;

    if (backendContext->vkFunctionTable.vkCreatePipelineLayout(backendContext->device, &pipelineLayoutInfo, nullptr, &pPipelineLayout->pipelineLayout) != VK_SUCCESS) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    // set the root signature to pipeline
    outPipeline->rootSignature = reinterpret_cast<FfxRootSignature>(pPipelineLayout);

    // Only set the command signature if this is setup as an indirect workload
    if (pipelineDescription->indirectWorkload)
    {
        // Only need the stride ahead of time in Vulkan
        outPipeline->cmdSignature = reinterpret_cast<FfxCommandSignature>(sizeof(VkDispatchIndirectCommand));
    }
    else
    {
        outPipeline->cmdSignature = nullptr;
    }

    uint32_t flattenedSrvTextureCount = 0;

    for (uint32_t srvIndex = 0; srvIndex < shaderBlob.srvTextureCount; ++srvIndex)
    {
        uint32_t slotIndex = shaderBlob.boundSRVTextures[srvIndex];
        uint32_t bindCount = shaderBlob.boundSRVTextureCounts[srvIndex];

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedSrvTextureCount++;

            outPipeline->srvTextureBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->srvTextureBindings[bindingIndex].arrayIndex = arrayIndex;
            ConvertUTF8ToUTF16(shaderBlob.boundSRVTextureNames[srvIndex], outPipeline->srvTextureBindings[bindingIndex].name, FFX_RESOURCE_NAME_SIZE);
        }
    }

    outPipeline->srvTextureCount = flattenedSrvTextureCount;
    FFX_ASSERT(outPipeline->srvTextureCount < FFX_MAX_NUM_SRVS);

    uint32_t flattenedUavTextureCount = 0;

    for (uint32_t uavIndex = 0; uavIndex < shaderBlob.uavTextureCount; ++uavIndex)
    {
        uint32_t slotIndex = shaderBlob.boundUAVTextures[uavIndex];
        uint32_t bindCount = shaderBlob.boundUAVTextureCounts[uavIndex];

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedUavTextureCount++;

            outPipeline->uavTextureBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->uavTextureBindings[bindingIndex].arrayIndex = arrayIndex;
            ConvertUTF8ToUTF16(shaderBlob.boundUAVTextureNames[uavIndex], outPipeline->uavTextureBindings[bindingIndex].name, FFX_RESOURCE_NAME_SIZE);
        }
    }

    outPipeline->uavTextureCount = flattenedUavTextureCount;
    FFX_ASSERT(outPipeline->uavTextureCount < FFX_MAX_NUM_UAVS);

    uint32_t flattenedSrvBufferCount = 0;

    for (uint32_t srvIndex = 0; srvIndex < shaderBlob.srvBufferCount; ++srvIndex)
    {
        uint32_t slotIndex  = shaderBlob.boundSRVBuffers[srvIndex];
        uint32_t spaceIndex = shaderBlob.boundSRVBufferSpaces[srvIndex];
        uint32_t bindCount  = shaderBlob.boundSRVBufferCounts[srvIndex];

        // Skip static resources
        if (spaceIndex == 1)
            continue;

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedSrvBufferCount++;

            outPipeline->srvBufferBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->srvBufferBindings[bindingIndex].arrayIndex = arrayIndex;
            ConvertUTF8ToUTF16(shaderBlob.boundSRVBufferNames[srvIndex], outPipeline->srvBufferBindings[bindingIndex].name, FFX_RESOURCE_NAME_SIZE);
        }
    }

    outPipeline->srvBufferCount = flattenedSrvBufferCount;
    FFX_ASSERT(outPipeline->srvBufferCount < FFX_MAX_NUM_SRVS);

    uint32_t flattenedUavBufferCount = 0;

    for (uint32_t uavIndex = 0; uavIndex < shaderBlob.uavBufferCount; ++uavIndex)
    {
        uint32_t slotIndex = shaderBlob.boundUAVBuffers[uavIndex];
        uint32_t bindCount = shaderBlob.boundUAVBufferCounts[uavIndex];

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedUavBufferCount++;

            outPipeline->uavBufferBindings[bindingIndex].slotIndex  = slotIndex;
            outPipeline->uavBufferBindings[bindingIndex].arrayIndex = arrayIndex;
            ConvertUTF8ToUTF16(shaderBlob.boundUAVBufferNames[uavIndex], outPipeline->uavBufferBindings[bindingIndex].name, FFX_RESOURCE_NAME_SIZE);
        }
    }

    outPipeline->uavBufferCount = flattenedUavBufferCount;
    FFX_ASSERT(outPipeline->uavBufferCount < FFX_MAX_NUM_UAVS);

    for (uint32_t cbIndex = 0; cbIndex < shaderBlob.cbvCount; ++cbIndex)
    {
        outPipeline->constantBufferBindings[cbIndex].slotIndex  = shaderBlob.boundConstantBuffers[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].arrayIndex = 1;
        ConvertUTF8ToUTF16(shaderBlob.boundConstantBufferNames[cbIndex], outPipeline->constantBufferBindings[cbIndex].name, FFX_RESOURCE_NAME_SIZE);
    }

    outPipeline->constCount = shaderBlob.cbvCount;
    FFX_ASSERT(outPipeline->constCount < FFX_MAX_NUM_CONST_BUFFERS);

    outPipeline->staticTextureSrvCount = staticTextureSrvCount;
    FFX_ASSERT(outPipeline->staticTextureSrvCount <= effectContext.bindlessTextureSrvHeapSize);

    outPipeline->staticBufferSrvCount = staticBufferSrvCount;
    FFX_ASSERT(outPipeline->staticBufferSrvCount <= effectContext.bindlessBufferSrvHeapSize);

    outPipeline->staticTextureUavCount = staticTextureUavCount;
    FFX_ASSERT(outPipeline->staticTextureUavCount <= effectContext.bindlessTextureUavHeapSize);

    outPipeline->staticBufferUavCount = staticBufferUavCount;
    FFX_ASSERT(outPipeline->staticBufferUavCount <= effectContext.bindlessBufferUavHeapSize);

    // Todo when needed
    //outPipeline->samplerCount      = shaderBlob.samplerCount;
    //outPipeline->rtAccelStructCount= shaderBlob.rtAccelStructCount;

    //////////////////////////////////////////////////////////////////////////
    // pipeline creation
    FfxDeviceCapabilities capabilities;
    backendInterface->fpGetDeviceCapabilities(backendInterface, &capabilities);

    // shader module
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pCode = (uint32_t*)shaderBlob.data;
    shaderModuleCreateInfo.codeSize = shaderBlob.size;

    if (backendContext->vkFunctionTable.vkCreateShaderModule(backendContext->device, &shaderModuleCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    // fill out shader stage create info
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.pName = "main";
    shaderStageCreateInfo.module = shaderModule;

    // check if wave64 is requested
    bool isWave64 = false;
    ffxIsWave64(effect, permutationOptions, isWave64);
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroupSizeCreateInfo = {};
    if (isWave64 && backendContext->subgroupSizeControlEnabled &&
        (capabilities.waveLaneCountMin <= 64 && capabilities.waveLaneCountMax >= 64))
    {
        subgroupSizeCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO_EXT;
        subgroupSizeCreateInfo.requiredSubgroupSize = 64;
        shaderStageCreateInfo.pNext = &subgroupSizeCreateInfo;
    }

    // create the compute pipeline
    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
    pipelineCreateInfo.layout = pPipelineLayout->pipelineLayout;

    VkPipeline computePipeline = VK_NULL_HANDLE;
    if (backendContext->vkFunctionTable.vkCreateComputePipelines(backendContext->device, nullptr, 1, &pipelineCreateInfo, nullptr, &computePipeline) != VK_SUCCESS) {
        return FFX_ERROR_BACKEND_API_ERROR;
    }

    // done with shader module, so clean up
    backendContext->vkFunctionTable.vkDestroyShaderModule(backendContext->device, shaderModule, nullptr);

    // set the pipeline
    outPipeline->pipeline = reinterpret_cast<FfxPipeline>(computePipeline);

    // Setup the pipeline name
    wcscpy_s(outPipeline->name, pipelineDescription->name);

    return FFX_OK;
}

FfxErrorCode DestroyPipelineVK(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId)
{
    FFX_ASSERT(backendInterface != nullptr);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    if (!pipeline)
        return FFX_OK;

    // Destroy the pipeline
    VkPipeline vkPipeline = reinterpret_cast<VkPipeline>(pipeline->pipeline);
    if (vkPipeline != VK_NULL_HANDLE) {
        backendContext->vkFunctionTable.vkDestroyPipeline(backendContext->device, vkPipeline, VK_NULL_HANDLE);
        pipeline->pipeline = VK_NULL_HANDLE;
    }

    // Zero out the cmd signature
    pipeline->cmdSignature = nullptr;

    // Destroy the pipeline layout
    BackendContext_VK::PipelineLayout* pPipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(pipeline->rootSignature);

    if (pPipelineLayout)
    {
        // Descriptor set layout
        if (pPipelineLayout->pipelineLayout != VK_NULL_HANDLE) {
            backendContext->vkFunctionTable.vkDestroyPipelineLayout(backendContext->device, pPipelineLayout->pipelineLayout, VK_NULL_HANDLE);
            pPipelineLayout->pipelineLayout = VK_NULL_HANDLE;
        }

        // Descriptor sets
        for (uint32_t i = 0; i < FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME; i++) {
            backendContext->vkFunctionTable.vkFreeDescriptorSets(backendContext->device, backendContext->descriptorPool, 1, &pPipelineLayout->descriptorSets[i]);
            pPipelineLayout->descriptorSets[i] = VK_NULL_HANDLE;
        }

        // Descriptor set layout
        if (pPipelineLayout->descriptorSetLayout != VK_NULL_HANDLE) {
            backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(backendContext->device, pPipelineLayout->descriptorSetLayout, VK_NULL_HANDLE);
            pPipelineLayout->descriptorSetLayout = VK_NULL_HANDLE;
        }

        // Samplers
        for (uint32_t currentSamplerIndex = 0; currentSamplerIndex < FFX_MAX_SAMPLERS; ++currentSamplerIndex)
        {
            if (pPipelineLayout->samplers[currentSamplerIndex] != VK_NULL_HANDLE) {
                backendContext->vkFunctionTable.vkDestroySampler(backendContext->device, pPipelineLayout->samplers[currentSamplerIndex], VK_NULL_HANDLE);
                pPipelineLayout->samplers[currentSamplerIndex] = VK_NULL_HANDLE;
            }
        }
    }

    return FFX_OK;
}

FfxErrorCode ScheduleGpuJobVK(FfxInterface* backendInterface, const FfxGpuJobDescription* job)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != job);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FFX_ASSERT(backendContext->gpuJobCount < FFX_MAX_GPU_JOBS);

    backendContext->pGpuJobs[backendContext->gpuJobCount] = *job;
    backendContext->gpuJobCount++;

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCompute(BackendContext_VK*    backendContext,
                                         FfxGpuJobDescription* job,
                                         VkCommandBuffer       vkCommandBuffer,
                                         FfxUInt32             effectContextId)
{
    BackendContext_VK::PipelineLayout* pipelineLayout = reinterpret_cast<BackendContext_VK::PipelineLayout*>(job->computeJobDescriptor.pipeline.rootSignature);

    // bind texture & buffer UAVs (note the binding order here MUST match the root signature mapping order from CreatePipeline!)
    uint32_t               descriptorWriteIndex = 0;
    VkWriteDescriptorSet   writeDescriptorSets[FFX_MAX_RESOURCE_COUNT];

    // These MUST be initialized
    uint32_t               imageDescriptorIndex = 0;
