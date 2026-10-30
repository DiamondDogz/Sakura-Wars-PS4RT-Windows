    VkDescriptorImageInfo  imageDescriptorInfos[FFX_MAX_RESOURCE_COUNT];
    for (int i = 0; i < FFX_MAX_RESOURCE_COUNT; ++i)
        imageDescriptorInfos[i] = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    // These MUST be initialized
    uint32_t               bufferDescriptorIndex = 0;
    VkDescriptorBufferInfo bufferDescriptorInfos[FFX_MAX_RESOURCE_COUNT];
    for (int i = 0; i < FFX_MAX_RESOURCE_COUNT; ++i)
        bufferDescriptorInfos[i] = { VK_NULL_HANDLE, 0, VK_WHOLE_SIZE };

    // bind texture UAVs
    for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavTextureCount; ++currentPipelineUavIndex)
    {
        FfxTextureUAV& textureUAV = job->computeJobDescriptor.uavTextures[currentPipelineUavIndex];

        // continue if this is a null resource.
        if (job->computeJobDescriptor.uavTextures[currentPipelineUavIndex].resource.internalIndex == 0)
            continue;

        addBarrier(backendContext, &textureUAV.resource, FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex];

        // where to bind it
        const uint32_t currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].slotIndex;

        // source: UAV of resource to bind
        const uint32_t resourceIndex = textureUAV.resource.internalIndex;
        uint32_t mipOffset = textureUAV.mip;
        if (textureUAV.mip >= backendContext->pResources[resourceIndex].resourceDescription.mipCount)
            mipOffset = backendContext->pResources[resourceIndex].resourceDescription.mipCount - 1;
        const uint32_t uavViewIndex  = backendContext->pResources[resourceIndex].uavViewIndex + mipOffset;

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writeDescriptorSets[descriptorWriteIndex].pImageInfo      = &imageDescriptorInfos[imageDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = binding.slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = binding.arrayIndex;

        imageDescriptorInfos[imageDescriptorIndex]             = {};
        imageDescriptorInfos[imageDescriptorIndex].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageDescriptorInfos[imageDescriptorIndex].imageView   = backendContext->pResourceViews[uavViewIndex].imageView;

        imageDescriptorIndex++;
        descriptorWriteIndex++;
    }

    // bind buffer UAVs
    for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavBufferCount; ++currentPipelineUavIndex) 
    {
        FfxBufferUAV& bufferUAV = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex];

        // continue if this is a null resource.
        if (job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].resource.internalIndex == 0)
            continue;

        addBarrier(backendContext, &bufferUAV.resource, FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex];

        // source: UAV of buffer to bind
        const uint32_t resourceIndex = bufferUAV.resource.internalIndex;

        // where to bind it
        const uint32_t currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex].slotIndex;

        writeDescriptorSets[descriptorWriteIndex] = {};
        writeDescriptorSets[descriptorWriteIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = binding.slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = binding.arrayIndex;

        bufferDescriptorInfos[bufferDescriptorIndex] = {};
        bufferDescriptorInfos[bufferDescriptorIndex].buffer = backendContext->pResources[resourceIndex].bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = bufferUAV.offset;
        bufferDescriptorInfos[bufferDescriptorIndex].range  = bufferUAV.size > 0 ? bufferUAV.size : VK_WHOLE_SIZE;

        bufferDescriptorIndex++;
        descriptorWriteIndex++;
    }

    // bind texture SRVs
    for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvTextureCount; ++currentPipelineSrvIndex)
    {
        FfxTextureSRV& textureSRV = job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex];

        // continue if this is a null resource.
        if (job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex].resource.internalIndex == 0)
            continue;

        addBarrier(backendContext, &textureSRV.resource, FFX_RESOURCE_STATE_COMPUTE_READ);

        const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex];

        // where to bind it
        const uint32_t currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].slotIndex;

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeDescriptorSets[descriptorWriteIndex].pImageInfo      = &imageDescriptorInfos[imageDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = binding.slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = binding.arrayIndex;

        const uint32_t resourceIndex = textureSRV.resource.internalIndex;
        const uint32_t srvViewIndex  = backendContext->pResources[resourceIndex].srvViewIndex;

        imageDescriptorInfos[imageDescriptorIndex]             = {};
        imageDescriptorInfos[imageDescriptorIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageDescriptorInfos[imageDescriptorIndex].imageView   = backendContext->pResourceViews[srvViewIndex].imageView;

        imageDescriptorIndex++;
        descriptorWriteIndex++;
    }

    // bind buffer SRVs
    for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvBufferCount; ++currentPipelineSrvIndex)
    {
        FfxBufferSRV& bufferSRV = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex];

        // continue if this is a null resource.
        if (job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].resource.internalIndex == 0)
            continue;

        addBarrier(backendContext, &bufferSRV.resource, FFX_RESOURCE_STATE_COMPUTE_READ);

        const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex];

        // source: SRV of buffer to bind
        const uint32_t resourceIndex = bufferSRV.resource.internalIndex;

        // where to bind it
        const uint32_t currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex].slotIndex;

        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo     = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding      = binding.slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = binding.arrayIndex;

        bufferDescriptorInfos[bufferDescriptorIndex]        = {};
        bufferDescriptorInfos[bufferDescriptorIndex].buffer = backendContext->pResources[resourceIndex].bufferResource;
        bufferDescriptorInfos[bufferDescriptorIndex].offset = bufferSRV.offset;
        bufferDescriptorInfos[bufferDescriptorIndex].range  = bufferSRV.size > 0 ? bufferSRV.size : VK_WHOLE_SIZE;
    
        bufferDescriptorIndex++;
        descriptorWriteIndex++;
    }

    // update uniform buffers
    for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < job->computeJobDescriptor.pipeline.constCount; ++currentRootConstantIndex)
    {
        uint32_t dataSize = job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t);
        
        // If we have a constant buffer allocator, use that, otherwise use the default backend allocator
        FfxConstantAllocation allocation;
        if (s_fpConstantAllocator)
            allocation = s_fpConstantAllocator(job->computeJobDescriptor.cbs[currentRootConstantIndex].data, dataSize);
        else
            allocation = backendContext->FallbackConstantAllocator(job->computeJobDescriptor.cbs[currentRootConstantIndex].data, dataSize);
            
        writeDescriptorSets[descriptorWriteIndex]                 = {};
        writeDescriptorSets[descriptorWriteIndex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[descriptorWriteIndex].dstSet          = pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex];
        writeDescriptorSets[descriptorWriteIndex].descriptorCount = 1;
        writeDescriptorSets[descriptorWriteIndex].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[descriptorWriteIndex].pBufferInfo     = &bufferDescriptorInfos[bufferDescriptorIndex];
        writeDescriptorSets[descriptorWriteIndex].dstBinding =
            job->computeJobDescriptor.pipeline.constantBufferBindings[currentRootConstantIndex].slotIndex;
        writeDescriptorSets[descriptorWriteIndex].dstArrayElement = 0;

        bufferDescriptorInfos[bufferDescriptorIndex].buffer = static_cast<VkBuffer>(allocation.resource.resource);
        bufferDescriptorInfos[bufferDescriptorIndex].offset = static_cast<VkDeviceSize>(allocation.handle);
        bufferDescriptorInfos[bufferDescriptorIndex].range  = dataSize;

        bufferDescriptorIndex++;
        descriptorWriteIndex++;
    }

    // If we are dispatching indirectly, transition the argument resource to indirect argument
    if (job->computeJobDescriptor.pipeline.cmdSignature)
    {
        addBarrier(backendContext, &job->computeJobDescriptor.cmdArgument, FFX_RESOURCE_STATE_INDIRECT_ARGUMENT);
    }

    // insert all the barriers
    flushBarriers(backendContext, vkCommandBuffer);

    // update all uavs and srvs
    backendContext->vkFunctionTable.vkUpdateDescriptorSets(backendContext->device, descriptorWriteIndex, writeDescriptorSets, 0, nullptr);

    // bind pipeline
    backendContext->vkFunctionTable.vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, reinterpret_cast<VkPipeline>(job->computeJobDescriptor.pipeline.pipeline));

    // bind descriptor sets
    {
        backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, 0, 1, &pipelineLayout->descriptorSets[pipelineLayout->descriptorSetIndex], 0, nullptr);

        BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

        if (job->computeJobDescriptor.pipeline.staticTextureSrvCount > 0)
            backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, pipelineLayout->staticTextureSrvSet, 1, &effectContext.bindlessTextureSrvDescriptorSet, 0, nullptr);

        if (job->computeJobDescriptor.pipeline.staticBufferSrvCount > 0)
            backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, pipelineLayout->staticBufferSrvSet, 1, &effectContext.bindlessBufferSrvDescriptorSet, 0, nullptr);

        if (job->computeJobDescriptor.pipeline.staticTextureUavCount > 0)
            backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, pipelineLayout->staticTextureUavSet, 1, &effectContext.bindlessTextureUavDescriptorSet, 0, nullptr);

        if (job->computeJobDescriptor.pipeline.staticBufferUavCount > 0)
            backendContext->vkFunctionTable.vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->pipelineLayout, pipelineLayout->staticBufferUavSet, 1, &effectContext.bindlessBufferUavDescriptorSet, 0, nullptr);
    }
    
    // Dispatch (or dispatch indirect)
    if (job->computeJobDescriptor.pipeline.cmdSignature)
    {
        const uint32_t resourceIndex = job->computeJobDescriptor.cmdArgument.internalIndex;
        VkBuffer buffer = backendContext->pResources[resourceIndex].bufferResource;
        backendContext->vkFunctionTable.vkCmdDispatchIndirect(vkCommandBuffer, buffer, job->computeJobDescriptor.cmdArgumentOffset);
    }
    else
    {
        backendContext->vkFunctionTable.vkCmdDispatch(vkCommandBuffer, job->computeJobDescriptor.dimensions[0], job->computeJobDescriptor.dimensions[1], job->computeJobDescriptor.dimensions[2]);
    }

    // move to another descriptor set for the next compute render job so that we don't overwrite descriptors in-use
    ++pipelineLayout->descriptorSetIndex;
    if (pipelineLayout->descriptorSetIndex >= (FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME))
        pipelineLayout->descriptorSetIndex = 0;

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCopy(BackendContext_VK* backendContext, FfxGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    BackendContext_VK::Resource ffxResourceSrc = backendContext->pResources[job->copyJobDescriptor.src.internalIndex];
    BackendContext_VK::Resource ffxResourceDst = backendContext->pResources[job->copyJobDescriptor.dst.internalIndex];

    addBarrier(backendContext, &job->copyJobDescriptor.src, FFX_RESOURCE_STATE_COPY_SRC);
    addBarrier(backendContext, &job->copyJobDescriptor.dst, FFX_RESOURCE_STATE_COPY_DEST);
    flushBarriers(backendContext, vkCommandBuffer);

    if (ffxResourceSrc.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER && ffxResourceDst.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResourceSrc = ffxResourceSrc.bufferResource;
        VkBuffer vkResourceDst = ffxResourceDst.bufferResource;

        VkBufferCopy bufferCopy = {};

        bufferCopy.dstOffset = job->copyJobDescriptor.dstOffset;
        bufferCopy.srcOffset = job->copyJobDescriptor.srcOffset;
        bufferCopy.size      = job->copyJobDescriptor.size > 0 ? job->copyJobDescriptor.size : ffxResourceSrc.resourceDescription.width;

        backendContext->vkFunctionTable.vkCmdCopyBuffer(vkCommandBuffer, vkResourceSrc, vkResourceDst, 1, &bufferCopy);
    }
    else if (ffxResourceSrc.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER && ffxResourceDst.resourceDescription.type != FFX_RESOURCE_TYPE_BUFFER)
    {
        VkBuffer vkResourceSrc = ffxResourceSrc.bufferResource;
        VkImage vkResourceDst = ffxResourceDst.imageResource;

        VkImageSubresourceLayers subresourceLayers = {};

        subresourceLayers.aspectMask = getImageAspect(ffxResourceDst.resourceDescription.usage);
        subresourceLayers.baseArrayLayer = 0;
        subresourceLayers.layerCount = 1;
        subresourceLayers.mipLevel = 0;

        VkOffset3D offset = {};

        offset.x = 0;
        offset.y = 0;
        offset.z = 0;

        VkExtent3D extent = {};

        extent.width = ffxResourceDst.resourceDescription.width;
        extent.height = ffxResourceDst.resourceDescription.height;
        extent.depth = ffxResourceDst.resourceDescription.depth;

        // TODO: account for source buffer offset
        VkBufferImageCopy bufferImageCopy = {};

        bufferImageCopy.bufferOffset = 0;
        bufferImageCopy.bufferRowLength = 0;
        bufferImageCopy.bufferImageHeight = 0;
        bufferImageCopy.imageSubresource = subresourceLayers;
        bufferImageCopy.imageOffset = offset;
        bufferImageCopy.imageExtent = extent;

        backendContext->vkFunctionTable.vkCmdCopyBufferToImage(vkCommandBuffer, vkResourceSrc, vkResourceDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferImageCopy);
    }
    else
    {
        bool isSrcDepth = FFX_CONTAINS_FLAG(ffxResourceSrc.resourceDescription.usage, FFX_RESOURCE_USAGE_DEPTHTARGET);
        bool isDstDepth = FFX_CONTAINS_FLAG(ffxResourceDst.resourceDescription.usage, FFX_RESOURCE_USAGE_DEPTHTARGET);
        FFX_ASSERT_MESSAGE(isSrcDepth == isDstDepth, "Copy operations aren't allowed between depth and color textures in the vulkan backend of the FFX SDK.");

        #define FFX_MAX_IMAGE_COPY_MIPS 14 // Will handle 4k down to 1x1
        VkImageCopy             imageCopies[FFX_MAX_IMAGE_COPY_MIPS];
        VkImage vkResourceSrc = ffxResourceSrc.imageResource;
        VkImage vkResourceDst = ffxResourceDst.imageResource;

        uint32_t numMipsToCopy = FFX_MINIMUM(ffxResourceSrc.resourceDescription.mipCount, ffxResourceDst.resourceDescription.mipCount);

        for (uint32_t mip = 0; mip < numMipsToCopy; mip++)
        {
            VkImageSubresourceLayers srcSubresourceLayers = {};

            srcSubresourceLayers.aspectMask     = getImageAspect(ffxResourceSrc.resourceDescription.usage);
            srcSubresourceLayers.baseArrayLayer = 0;
            srcSubresourceLayers.layerCount     = 1;
            srcSubresourceLayers.mipLevel       = mip;

            VkImageSubresourceLayers dstSubresourceLayers = {};

            dstSubresourceLayers.aspectMask     = getImageAspect(ffxResourceDst.resourceDescription.usage);
            dstSubresourceLayers.baseArrayLayer = 0;
            dstSubresourceLayers.layerCount     = 1;
            dstSubresourceLayers.mipLevel       = mip;

            VkOffset3D offset = {};

            offset.x = 0;
            offset.y = 0;
            offset.z = 0;

            VkExtent3D extent = {};

            extent.width = ffxResourceSrc.resourceDescription.width / (mip + 1);
            extent.height = ffxResourceSrc.resourceDescription.height / (mip + 1);
            extent.depth = ffxResourceSrc.resourceDescription.depth / (mip + 1);

            VkImageCopy& copyRegion = imageCopies[mip];

            copyRegion.srcSubresource = srcSubresourceLayers;
            copyRegion.srcOffset = offset;
            copyRegion.dstSubresource = dstSubresourceLayers;
            copyRegion.dstOffset = offset;
            copyRegion.extent = extent;
        }

        backendContext->vkFunctionTable.vkCmdCopyImage(vkCommandBuffer, vkResourceSrc, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkResourceDst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, numMipsToCopy, imageCopies);
    }

    return FFX_OK;
}

static FfxErrorCode executeGpuJobBarrier(BackendContext_VK* backendContext, FfxGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    addBarrier(backendContext, &job->barrierDescriptor.resource, job->barrierDescriptor.newState);
    flushBarriers(backendContext, vkCommandBuffer);

    return FFX_OK;
}

static FfxErrorCode executeGpuJobTimestamp(BackendContext_VK* backendContext, FfxGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    return FFX_OK;
}

static FfxErrorCode executeGpuJobClearFloat(BackendContext_VK* backendContext, FfxGpuJobDescription* job, VkCommandBuffer vkCommandBuffer)
{
    uint32_t idx = job->clearJobDescriptor.target.internalIndex;
    BackendContext_VK::Resource ffxResource = backendContext->pResources[idx];

    if (ffxResource.resourceDescription.type == FFX_RESOURCE_TYPE_BUFFER)
    {
        addBarrier(backendContext, &job->clearJobDescriptor.target, FFX_RESOURCE_STATE_COPY_DEST);
        flushBarriers(backendContext, vkCommandBuffer);

        VkBuffer vkResource = ffxResource.bufferResource;

        backendContext->vkFunctionTable.vkCmdFillBuffer(vkCommandBuffer, vkResource, 0, VK_WHOLE_SIZE, (uint32_t)job->clearJobDescriptor.color[0]);
    }
    else
    {
        addBarrier(backendContext, &job->clearJobDescriptor.target, FFX_RESOURCE_STATE_COPY_DEST);
        flushBarriers(backendContext, vkCommandBuffer);

        VkImage vkResource = ffxResource.imageResource;

        VkClearColorValue clearColorValue = {};

        clearColorValue.float32[0] = job->clearJobDescriptor.color[0];
        clearColorValue.float32[1] = job->clearJobDescriptor.color[1];
        clearColorValue.float32[2] = job->clearJobDescriptor.color[2];
        clearColorValue.float32[3] = job->clearJobDescriptor.color[3];

        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = ffxResource.resourceDescription.mipCount;
        range.baseArrayLayer = 0;
        if (ffxResource.resourceDescription.type == FFX_RESOURCE_TYPE_TEXTURE3D)
            range.layerCount = 1;
        else
            range.layerCount = ffxResource.resourceDescription.depth;  // in that case depth is the number of layers
        
        backendContext->vkFunctionTable.vkCmdClearColorImage(vkCommandBuffer, vkResource, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColorValue, 1, &range);
    }

    return FFX_OK;
}

FfxErrorCode ExecuteGpuJobsVK(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    FFX_ASSERT(nullptr != commandList);
    VkCommandBuffer vkCommandBuffer = reinterpret_cast<VkCommandBuffer>(commandList);

    FfxErrorCode errorCode = FFX_OK;

    // execute all renderjobs
    for (uint32_t i = 0; i < backendContext->gpuJobCount; ++i)
    {
        FfxGpuJobDescription* gpuJob = &backendContext->pGpuJobs[i];

        // If we have a label for the job, drop a marker for it
        if (gpuJob->jobLabel[0]) {
            beginMarkerVK(backendContext, vkCommandBuffer, gpuJob->jobLabel);
        }

        
        switch (gpuJob->jobType)
        {
        case FFX_GPU_JOB_CLEAR_FLOAT:
        {
            errorCode = executeGpuJobClearFloat(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        case FFX_GPU_JOB_COPY:
        {
            errorCode = executeGpuJobCopy(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        case FFX_GPU_JOB_COMPUTE:
        {
            errorCode = executeGpuJobCompute(backendContext, gpuJob, vkCommandBuffer, effectContextId);
            break;
        }
        case FFX_GPU_JOB_BARRIER:
        {
            errorCode = executeGpuJobBarrier(backendContext, gpuJob, vkCommandBuffer);
            break;
        }
        default:;
        }

        if (gpuJob->jobLabel[0]) {
            endMarkerVK(backendContext, vkCommandBuffer);
        }
    }

    // check the execute function returned cleanly.
    FFX_RETURN_ON_ERROR(
        errorCode == FFX_OK,
        FFX_ERROR_BACKEND_API_ERROR);

    backendContext->gpuJobCount = 0;

    return FFX_OK;
}

FfxErrorCode BreadcrumbsAllocBlockVK(
    FfxInterface* backendInterface,
    uint64_t blockBytes,
    FfxBreadcrumbsBlockData* blockData)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != blockData);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    void* mappedMemory = nullptr;
    uint64_t baseAddress = 0;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.flags = 0;
    bufferInfo.size = blockBytes;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.pQueueFamilyIndices = nullptr;

    VkBuffer buffer = VK_NULL_HANDLE;
    if (backendContext->vkFunctionTable.vkCreateBuffer(backendContext->device, &bufferInfo, nullptr, &buffer) == VK_SUCCESS)
    {
        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr };
        allocInfo.allocationSize = blockBytes;
        allocInfo.memoryTypeIndex = backendContext->breadcrumbsMemoryIndex;

        VkMemoryDedicatedAllocateInfo dedicatedAlloc = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, nullptr };
        if (FFX_CONTAINS_FLAG(backendContext->breadcrumbsFlags, BackendContext_VK::BREADCRUMBS_DEDICATED_MEMORY_ENABLED))
        {
            dedicatedAlloc.image = VK_NULL_HANDLE;
            dedicatedAlloc.buffer = buffer;
            dedicatedAlloc.pNext = allocInfo.pNext;
            allocInfo.pNext = &dedicatedAlloc;
        }

        VkDeviceMemory memory = VK_NULL_HANDLE;
        if (backendContext->vkFunctionTable.vkAllocateMemory(backendContext->device, &allocInfo, nullptr, &memory) == VK_SUCCESS)
        {
            if (backendContext->vkFunctionTable.vkBindBufferMemory(backendContext->device, buffer, memory, 0) == VK_SUCCESS)
            {
                if (mappedMemory || backendContext->vkFunctionTable.vkMapMemory(backendContext->device, memory, 0, blockBytes, 0, &mappedMemory) == VK_SUCCESS)
                {
                    blockData->memory = mappedMemory;
                    blockData->heap = (void*)memory;
                    blockData->buffer = (void*)buffer;
                    blockData->baseAddress = baseAddress;
                    return FFX_OK;
                }
            }
            backendContext->vkFunctionTable.vkFreeMemory(backendContext->device, memory, nullptr);
        }
    }
    backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, buffer, nullptr);
    return FFX_ERROR_BACKEND_API_ERROR;
}

void BreadcrumbsFreeBlockVK(
    FfxInterface* backendInterface,
    FfxBreadcrumbsBlockData* blockData)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != blockData);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    if (blockData->memory && blockData->baseAddress == 0)
    {
        backendContext->vkFunctionTable.vkUnmapMemory(backendContext->device, (VkDeviceMemory)blockData->heap);
        blockData->memory = nullptr;
    }
    if (blockData->buffer)
    {
        backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, (VkBuffer)blockData->buffer, nullptr);
        blockData->buffer = nullptr;
    }
    if (blockData->heap)
    {
        backendContext->vkFunctionTable.vkFreeMemory(backendContext->device, (VkDeviceMemory)blockData->heap, nullptr);
        blockData->heap = nullptr;
    }
}

void BreadcrumbsWriteVK(
    FfxInterface* backendInterface,
    FfxCommandList commandList,
    uint32_t value,
    uint64_t gpuLocation,
    void* gpuBuffer,
    bool isBegin)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != gpuBuffer);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    if (FFX_CONTAINS_FLAG(backendContext->breadcrumbsFlags, BackendContext_VK::BREADCRUMBS_BUFFER_MARKER_ENABLED))
    {
        if (FFX_CONTAINS_FLAG(backendContext->breadcrumbsFlags, BackendContext_VK::BREADCRUMBS_SYNCHRONIZATION2_ENABLED))
        {
            backendContext->vkFunctionTable.vkCmdWriteBufferMarker2AMD((VkCommandBuffer)commandList,
                isBegin ? VK_PIPELINE_STAGE_2_NONE_KHR : VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR,
                (VkBuffer)gpuBuffer, gpuLocation, value);
        }
        else
        {
            backendContext->vkFunctionTable.vkCmdWriteBufferMarkerAMD((VkCommandBuffer)commandList,
                isBegin ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                (VkBuffer)gpuBuffer, gpuLocation, value);
        }
    }
    else
    {
        backendContext->vkFunctionTable.vkCmdFillBuffer((VkCommandBuffer)commandList, (VkBuffer)gpuBuffer, gpuLocation, sizeof(uint32_t), value);
    }
}

void BreadcrumbsPrintDeviceInfoVK(
    FfxInterface* backendInterface,
    FfxAllocationCallbacks* allocs,
    bool extendedInfo,
    char** printBuffer,
    size_t* printSize)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != allocs);
    FFX_ASSERT(NULL != printBuffer);
    FFX_ASSERT(NULL != printSize);

    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    char* buff = *printBuffer;
    size_t buffSize = *printSize;

    VkPhysicalDeviceProperties devProps = {};
    VkPhysicalDeviceFeatures devFeatures = {};
    vkGetPhysicalDeviceProperties(backendContext->physicalDevice, &devProps);
    vkGetPhysicalDeviceFeatures(backendContext->physicalDevice, &devFeatures);

    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "[VkPhysicalDeviceProperties]\n" FFX_BREADCRUMBS_PRINTING_INDENT "apiVersion: ");
    FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, VK_API_VERSION_MAJOR(devProps.apiVersion));
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, ".");
    FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, VK_API_VERSION_MINOR(devProps.apiVersion));
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, ".");
    FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, VK_API_VERSION_PATCH(devProps.apiVersion));
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "\n");

    FFX_BREADCRUMBS_PRINT_HEX32(buff, buffSize, devProps, driverVersion);
    FFX_BREADCRUMBS_PRINT_HEX32(buff, buffSize, devProps, vendorID);
    FFX_BREADCRUMBS_PRINT_HEX32(buff, buffSize, devProps, deviceID);

    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT "deviceType: ");
    switch (devProps.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
    {
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "OTHER\n");
        break;
    }
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
    {
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "INTEGRATED_GPU\n");
        break;
    }
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    {
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "DISCRETE_GPU\n");
        break;
    }
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
    {
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "VIRTUAL_GPU\n");
        break;
    }
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
    {
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "CPU\n");
        break;
    }
    default:
    {
        FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, devProps.deviceType);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "\n");
        break;
    }
    }

    FFX_BREADCRUMBS_PRINT_STRING(buff, buffSize, devProps, deviceName);
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT "pipelineCacheUUID: ");
    FFX_BREADCRUMBS_PRINT_HEX_BYTE(buff, buffSize, devProps.pipelineCacheUUID[0]);
    FFX_BREADCRUMBS_PRINT_HEX_BYTE(buff, buffSize, devProps.pipelineCacheUUID[1]);
    FFX_BREADCRUMBS_PRINT_HEX_BYTE(buff, buffSize, devProps.pipelineCacheUUID[2]);
    FFX_BREADCRUMBS_PRINT_HEX_BYTE(buff, buffSize, devProps.pipelineCacheUUID[3]);

    for (uint8_t i = 4; i < 12; i += 2)
    {
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "-");
        FFX_BREADCRUMBS_PRINT_HEX_BYTE(buff, buffSize, devProps.pipelineCacheUUID[i]);
        FFX_BREADCRUMBS_PRINT_HEX_BYTE(buff, buffSize, devProps.pipelineCacheUUID[i + 1]);
    }

    FFX_BREADCRUMBS_PRINT_HEX_BYTE(buff, buffSize, devProps.pipelineCacheUUID[12]);
    FFX_BREADCRUMBS_PRINT_HEX_BYTE(buff, buffSize, devProps.pipelineCacheUUID[13]);
    FFX_BREADCRUMBS_PRINT_HEX_BYTE(buff, buffSize, devProps.pipelineCacheUUID[14]);
    FFX_BREADCRUMBS_PRINT_HEX_BYTE(buff, buffSize, devProps.pipelineCacheUUID[15]);

    // Helper for printing device limits uint32_t.
#define BREAD_PRINT_LIMIT(name) \
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT); \
    FFX_BREADCRUMBS_PRINT_UINT(buff, buffSize, devProps.limits, name)
    // Helper for printing device limits uint64_t.
#define BREAD_PRINT_LIMIT64(name) \
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT); \
    FFX_BREADCRUMBS_PRINT_UINT64(buff, buffSize, devProps.limits, name)
    // Helper for printing device limits uint64_t.
#define BREAD_PRINT_LIMIT_FLOAT(name) \
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT); \
    FFX_BREADCRUMBS_PRINT_FLOAT(buff, buffSize, devProps.limits, name)
    // Helper for printing device limits as 32bit hexadecimal.
#define BREAD_PRINT_LIMIT_HEX(name) \
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT); \
    FFX_BREADCRUMBS_PRINT_HEX32(buff, buffSize, devProps.limits, name)

    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "\n" FFX_BREADCRUMBS_PRINTING_INDENT "limits: [VkPhysicalDeviceLimits]\n");
    BREAD_PRINT_LIMIT(maxImageDimension1D);
    BREAD_PRINT_LIMIT(maxImageDimension2D);
    BREAD_PRINT_LIMIT(maxImageDimension3D);
    BREAD_PRINT_LIMIT(maxImageDimensionCube);
    BREAD_PRINT_LIMIT(maxImageArrayLayers);
    BREAD_PRINT_LIMIT(maxTexelBufferElements);
    BREAD_PRINT_LIMIT(maxUniformBufferRange);
    BREAD_PRINT_LIMIT(maxStorageBufferRange);
    BREAD_PRINT_LIMIT(maxPushConstantsSize);
    BREAD_PRINT_LIMIT(maxMemoryAllocationCount);
    BREAD_PRINT_LIMIT(maxSamplerAllocationCount);
    BREAD_PRINT_LIMIT64(bufferImageGranularity);
    BREAD_PRINT_LIMIT64(sparseAddressSpaceSize);

    if (extendedInfo)
    {
        BREAD_PRINT_LIMIT(maxBoundDescriptorSets);
        BREAD_PRINT_LIMIT(maxPerStageDescriptorSamplers);
        BREAD_PRINT_LIMIT(maxPerStageDescriptorUniformBuffers);
        BREAD_PRINT_LIMIT(maxPerStageDescriptorStorageBuffers);
        BREAD_PRINT_LIMIT(maxPerStageDescriptorSampledImages);
        BREAD_PRINT_LIMIT(maxPerStageDescriptorStorageImages);
        BREAD_PRINT_LIMIT(maxPerStageDescriptorInputAttachments);
        BREAD_PRINT_LIMIT(maxPerStageResources);
        BREAD_PRINT_LIMIT(maxDescriptorSetSamplers);
        BREAD_PRINT_LIMIT(maxDescriptorSetUniformBuffers);
        BREAD_PRINT_LIMIT(maxDescriptorSetUniformBuffersDynamic);
        BREAD_PRINT_LIMIT(maxDescriptorSetStorageBuffers);
        BREAD_PRINT_LIMIT(maxDescriptorSetStorageBuffersDynamic);
        BREAD_PRINT_LIMIT(maxDescriptorSetSampledImages);
        BREAD_PRINT_LIMIT(maxDescriptorSetStorageImages);
        BREAD_PRINT_LIMIT(maxDescriptorSetInputAttachments);
        BREAD_PRINT_LIMIT(maxVertexInputAttributes);
        BREAD_PRINT_LIMIT(maxVertexInputBindings);
        BREAD_PRINT_LIMIT(maxVertexInputAttributeOffset);
        BREAD_PRINT_LIMIT(maxVertexInputBindingStride);
        BREAD_PRINT_LIMIT(maxVertexOutputComponents);
        BREAD_PRINT_LIMIT(maxTessellationGenerationLevel);
        BREAD_PRINT_LIMIT(maxTessellationPatchSize);
        BREAD_PRINT_LIMIT(maxTessellationControlPerVertexInputComponents);
        BREAD_PRINT_LIMIT(maxTessellationControlPerVertexOutputComponents);
        BREAD_PRINT_LIMIT(maxTessellationControlPerPatchOutputComponents);
        BREAD_PRINT_LIMIT(maxTessellationControlTotalOutputComponents);
        BREAD_PRINT_LIMIT(maxTessellationEvaluationInputComponents);
        BREAD_PRINT_LIMIT(maxTessellationEvaluationOutputComponents);
        BREAD_PRINT_LIMIT(maxGeometryShaderInvocations);
        BREAD_PRINT_LIMIT(maxGeometryInputComponents);
        BREAD_PRINT_LIMIT(maxGeometryOutputComponents);
        BREAD_PRINT_LIMIT(maxGeometryTotalOutputComponents);
        BREAD_PRINT_LIMIT(maxFragmentInputComponents);
        BREAD_PRINT_LIMIT(maxFragmentOutputAttachments);
        BREAD_PRINT_LIMIT(maxFragmentDualSrcAttachments);
        BREAD_PRINT_LIMIT(maxFragmentCombinedOutputResources);
        BREAD_PRINT_LIMIT(maxComputeSharedMemorySize);

        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT FFX_BREADCRUMBS_PRINTING_INDENT "maxComputeWorkGroupCount: [ ");
        FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, devProps.limits.maxComputeWorkGroupCount[0]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, ", ");
        FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, devProps.limits.maxComputeWorkGroupCount[1]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, ", ");
        FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, devProps.limits.maxComputeWorkGroupCount[2]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, " ]\n");

        BREAD_PRINT_LIMIT(maxComputeWorkGroupInvocations);

        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT FFX_BREADCRUMBS_PRINTING_INDENT "maxComputeWorkGroupSize: [ ");
        FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, devProps.limits.maxComputeWorkGroupSize[0]);
