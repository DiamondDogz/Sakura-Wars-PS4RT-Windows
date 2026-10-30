        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, ", ");
        FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, devProps.limits.maxComputeWorkGroupSize[1]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, ", ");
        FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, devProps.limits.maxComputeWorkGroupSize[2]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, " ]\n");

        BREAD_PRINT_LIMIT(subPixelPrecisionBits);
        BREAD_PRINT_LIMIT(subTexelPrecisionBits);
        BREAD_PRINT_LIMIT(mipmapPrecisionBits);
        BREAD_PRINT_LIMIT(maxDrawIndexedIndexValue);
        BREAD_PRINT_LIMIT(maxDrawIndirectCount);
        BREAD_PRINT_LIMIT_FLOAT(maxSamplerLodBias);
        BREAD_PRINT_LIMIT_FLOAT(maxSamplerAnisotropy);
        BREAD_PRINT_LIMIT(maxViewports);

        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT FFX_BREADCRUMBS_PRINTING_INDENT "maxViewportDimensions: [ ");
        FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, devProps.limits.maxViewportDimensions[0]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, ", ");
        FFX_BREADCRUMBS_APPEND_UINT(buff, buffSize, devProps.limits.maxViewportDimensions[1]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, " ]\n" FFX_BREADCRUMBS_PRINTING_INDENT FFX_BREADCRUMBS_PRINTING_INDENT "viewportBoundsRange: [ ");
        FFX_BREADCRUMBS_APPEND_FLOAT(buff, buffSize, devProps.limits.viewportBoundsRange[0]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, ", ");
        FFX_BREADCRUMBS_APPEND_FLOAT(buff, buffSize, devProps.limits.viewportBoundsRange[1]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, " ]\n");

        BREAD_PRINT_LIMIT(viewportSubPixelBits);
        BREAD_PRINT_LIMIT64(minMemoryMapAlignment);
        BREAD_PRINT_LIMIT64(minMemoryMapAlignment);
        BREAD_PRINT_LIMIT64(minTexelBufferOffsetAlignment);
        BREAD_PRINT_LIMIT64(minUniformBufferOffsetAlignment);
        BREAD_PRINT_LIMIT64(minStorageBufferOffsetAlignment);
        BREAD_PRINT_LIMIT(minTexelOffset);
        BREAD_PRINT_LIMIT(maxTexelOffset);
        BREAD_PRINT_LIMIT(minTexelGatherOffset);
        BREAD_PRINT_LIMIT(maxTexelGatherOffset);
        BREAD_PRINT_LIMIT_FLOAT(minInterpolationOffset);
        BREAD_PRINT_LIMIT_FLOAT(maxInterpolationOffset);
        BREAD_PRINT_LIMIT(subPixelInterpolationOffsetBits);
        BREAD_PRINT_LIMIT(maxFramebufferWidth);
        BREAD_PRINT_LIMIT(maxFramebufferHeight);
        BREAD_PRINT_LIMIT(maxFramebufferLayers);
        BREAD_PRINT_LIMIT_HEX(framebufferColorSampleCounts);
        BREAD_PRINT_LIMIT_HEX(framebufferDepthSampleCounts);
        BREAD_PRINT_LIMIT_HEX(framebufferStencilSampleCounts);
        BREAD_PRINT_LIMIT_HEX(framebufferNoAttachmentsSampleCounts);
        BREAD_PRINT_LIMIT(maxColorAttachments);
        BREAD_PRINT_LIMIT_HEX(sampledImageColorSampleCounts);
        BREAD_PRINT_LIMIT_HEX(sampledImageIntegerSampleCounts);
        BREAD_PRINT_LIMIT_HEX(sampledImageDepthSampleCounts);
        BREAD_PRINT_LIMIT_HEX(sampledImageStencilSampleCounts);
        BREAD_PRINT_LIMIT_HEX(storageImageSampleCounts);
        BREAD_PRINT_LIMIT(maxSampleMaskWords);

        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devProps.limits, timestampComputeAndGraphics);

        BREAD_PRINT_LIMIT_FLOAT(timestampPeriod);
        BREAD_PRINT_LIMIT(maxClipDistances);
        BREAD_PRINT_LIMIT(maxCullDistances);
        BREAD_PRINT_LIMIT(maxCombinedClipAndCullDistances);
        BREAD_PRINT_LIMIT(discreteQueuePriorities);

        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT FFX_BREADCRUMBS_PRINTING_INDENT "pointSizeRange: [ ");
        FFX_BREADCRUMBS_APPEND_FLOAT(buff, buffSize, devProps.limits.pointSizeRange[0]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, ", ");
        FFX_BREADCRUMBS_APPEND_FLOAT(buff, buffSize, devProps.limits.pointSizeRange[1]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, " ]\n" FFX_BREADCRUMBS_PRINTING_INDENT FFX_BREADCRUMBS_PRINTING_INDENT "lineWidthRange: [ ");
        FFX_BREADCRUMBS_APPEND_FLOAT(buff, buffSize, devProps.limits.lineWidthRange[0]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, ", ");
        FFX_BREADCRUMBS_APPEND_FLOAT(buff, buffSize, devProps.limits.lineWidthRange[1]);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, " ]\n");

        BREAD_PRINT_LIMIT_FLOAT(pointSizeGranularity);
        BREAD_PRINT_LIMIT_FLOAT(lineWidthGranularity);

        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devProps.limits, strictLines);
        FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devProps.limits, standardSampleLocations);

        BREAD_PRINT_LIMIT64(optimalBufferCopyOffsetAlignment);
        BREAD_PRINT_LIMIT64(optimalBufferCopyRowPitchAlignment);
        BREAD_PRINT_LIMIT64(nonCoherentAtomSize);
    }
#undef BREAD_PRINT_LIMIT
#undef BREAD_PRINT_LIMIT64
#undef BREAD_PRINT_LIMIT_HEX

    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT "sparseProperties: [VkPhysicalDeviceSparseProperties]\n" FFX_BREADCRUMBS_PRINTING_INDENT);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devProps.sparseProperties, residencyStandard2DBlockShape);
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devProps.sparseProperties, residencyStandard2DMultisampleBlockShape);
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devProps.sparseProperties, residencyStandard3DBlockShape);
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devProps.sparseProperties, residencyAlignedMipSize);
    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, FFX_BREADCRUMBS_PRINTING_INDENT);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devProps.sparseProperties, residencyNonResidentStrict);

    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "[VkPhysicalDeviceFeatures]\n");
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, robustBufferAccess);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, fullDrawIndexUint32);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, imageCubeArray);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, independentBlend);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, geometryShader);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, tessellationShader);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, sampleRateShading);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, dualSrcBlend);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, logicOp);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, multiDrawIndirect);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, drawIndirectFirstInstance);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, depthClamp);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, depthBiasClamp);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, fillModeNonSolid);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, depthBounds);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, wideLines);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, largePoints);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, alphaToOne);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, multiViewport);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, samplerAnisotropy);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, textureCompressionETC2);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, textureCompressionASTC_LDR);
    FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, textureCompressionBC);

    if (extendedInfo)
    {
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, occlusionQueryPrecise);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, pipelineStatisticsQuery);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, vertexPipelineStoresAndAtomics);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, fragmentStoresAndAtomics);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderTessellationAndGeometryPointSize);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderImageGatherExtended);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderStorageImageExtendedFormats);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderStorageImageMultisample);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderStorageImageReadWithoutFormat);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderStorageImageWriteWithoutFormat);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderUniformBufferArrayDynamicIndexing);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderSampledImageArrayDynamicIndexing);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderStorageBufferArrayDynamicIndexing);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderStorageImageArrayDynamicIndexing);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderClipDistance);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderCullDistance);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderFloat64);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderInt64);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderInt16);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderResourceResidency);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, shaderResourceMinLod);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, sparseBinding);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, sparseResidencyBuffer);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, sparseResidencyImage2D);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, sparseResidencyImage3D);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, sparseResidency2Samples);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, sparseResidency4Samples);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, sparseResidency8Samples);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, sparseResidency16Samples);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, sparseResidencyAliased);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, variableMultisampleRate);
        FFX_BREADCRUMBS_PRINT_BOOL(buff, buffSize, devFeatures, inheritedQueries);
    }

    FFX_BREADCRUMBS_APPEND_STRING(buff, buffSize, "\n");
    *printBuffer = buff;
    *printSize = buffSize;
}

void RegisterConstantBufferAllocatorVK(FfxInterface*, FfxConstantBufferAllocator fpConstantAllocator)
{
    s_fpConstantAllocator = fpConstantAllocator;
}

FfxCommandQueue ffxGetCommandQueueVK(VkQueue commandQueue)
{
    FFX_ASSERT(commandQueue != VK_NULL_HANDLE);
    return reinterpret_cast<FfxCommandQueue>(commandQueue);
}

FfxSwapchain ffxGetSwapchainVK(VkSwapchainKHR swapchain)
{
    FFX_ASSERT(swapchain != VK_NULL_HANDLE);
    return reinterpret_cast<FfxSwapchain>(swapchain);
}

VkSwapchainKHR ffxGetVKSwapchain(FfxSwapchain ffxSwapchain)
{
    return reinterpret_cast<VkSwapchainKHR>(ffxSwapchain);
}
