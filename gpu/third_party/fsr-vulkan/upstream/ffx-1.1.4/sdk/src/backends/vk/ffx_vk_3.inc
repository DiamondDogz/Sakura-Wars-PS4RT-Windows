            backendContext->vkFunctionTable.vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkDeviceContext->vkDeviceProcAddr(backendContext->device, "vkCmdEndDebugUtilsLabelEXT");
        }

        // enumerate all the device extensions
        backendContext->numDeviceExtensions = 0;
        vkEnumerateDeviceExtensionProperties(backendContext->physicalDevice, nullptr, &backendContext->numDeviceExtensions, nullptr);
        vkEnumerateDeviceExtensionProperties(backendContext->physicalDevice, nullptr, &backendContext->numDeviceExtensions, backendContext->extensionProperties);

        // create a global descriptor pool to hold all descriptors we'll need
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES * MAX_PIPELINE_USAGE_PER_FRAME },
        };

        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext = nullptr;
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(FFX_ARRAY_ELEMENTS(poolSizes));
        descriptorPoolCreateInfo.pPoolSizes = poolSizes;
        descriptorPoolCreateInfo.maxSets = backendContext->maxEffectContexts * FFX_MAX_PASS_COUNT * MAX_PIPELINE_USAGE_PER_FRAME * FFX_MAX_QUEUED_FRAMES;

        if (backendContext->vkFunctionTable.vkCreateDescriptorPool(backendContext->device, &descriptorPoolCreateInfo, nullptr, &backendContext->descriptorPool) != VK_SUCCESS) {
            return FFX_ERROR_BACKEND_API_ERROR;
        }

        // set bindless resource view to base
        backendContext->bindlessBase = (backendContext->maxEffectContexts * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2);

        // allocate dynamic uniform buffer
        {
            // get alignment
            VkPhysicalDeviceProperties physicalDeviceProperties = {};
            vkGetPhysicalDeviceProperties(backendContext->physicalDevice, &physicalDeviceProperties);
            backendContext->uniformBufferAlignment = physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

            backendContext->uniformBufferSize =
                FFX_ALIGN_UP(FFX_BUFFER_SIZE, backendContext->uniformBufferAlignment) * backendContext->maxEffectContexts * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES;

            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size               = static_cast<VkDeviceSize>(backendContext->uniformBufferSize);
            bufferInfo.usage              = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

            if (backendContext->vkFunctionTable.vkCreateBuffer(backendContext->device, &bufferInfo, NULL, &backendContext->uniformBuffer) != VK_SUCCESS)
            {
                return FFX_ERROR_BACKEND_API_ERROR;
            }

            // allocate memory block for all uniform buffers
            VkMemoryRequirements memRequirements = {};
            backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, backendContext->uniformBuffer, &memRequirements);

            // this is the real alignment
            backendContext->uniformBufferAlignment = memRequirements.alignment;

            VkMemoryPropertyFlags requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = backendContext->uniformBufferSize;
            allocInfo.memoryTypeIndex = findMemoryTypeIndex(
                backendContext->physicalDevice,
                memRequirements,
                requiredMemoryProperties,
                backendContext->deviceCoherentMemoryEnabled,
                backendContext->uniformBufferMemoryProperties);

            if (allocInfo.memoryTypeIndex == UINT32_MAX)
            {
                requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                allocInfo.memoryTypeIndex = findMemoryTypeIndex(
                    backendContext->physicalDevice,
                    memRequirements,
                    requiredMemoryProperties,
                    backendContext->deviceCoherentMemoryEnabled,
                    backendContext->uniformBufferMemoryProperties);

                if (allocInfo.memoryTypeIndex == UINT32_MAX)
                {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            }

            VkResult result = backendContext->vkFunctionTable.vkAllocateMemory(backendContext->device, &allocInfo, nullptr, &backendContext->uniformBufferMemory);

            if (result != VK_SUCCESS)
            {
                switch (result)
                {
                case (VK_ERROR_OUT_OF_HOST_MEMORY):
                case (VK_ERROR_OUT_OF_DEVICE_MEMORY):
                    return FFX_ERROR_OUT_OF_MEMORY;
                default:
                    return FFX_ERROR_BACKEND_API_ERROR;
                }
            }

            // map the memory block
            if (backendContext->vkFunctionTable.vkMapMemory(
                    backendContext->device, backendContext->uniformBufferMemory, 0, backendContext->uniformBufferSize, 0, &backendContext->uniformBufferMem) !=
                VK_SUCCESS)
            {
                return FFX_ERROR_BACKEND_API_ERROR;
            }

            if (backendContext->vkFunctionTable.vkBindBufferMemory(
                    backendContext->device, backendContext->uniformBuffer, backendContext->uniformBufferMemory, 0) != VK_SUCCESS)
            {
                return FFX_ERROR_BACKEND_API_ERROR;
            }
        }

        // Setup Breadcrumbs data
        {
            FfxDeviceCapabilities devCaps = {};
            if (GetDeviceCapabilitiesVK(backendInterface, &devCaps) != FFX_OK)
                return FFX_ERROR_BACKEND_API_ERROR;

            // Get info for memory used as Breadcrumbs buffer
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.pNext = nullptr;
            bufferInfo.flags = 0;
            bufferInfo.size = 256;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            bufferInfo.queueFamilyIndexCount = 0;
            bufferInfo.pQueueFamilyIndices = nullptr;

            VkBuffer testBuffer = VK_NULL_HANDLE;
            if (vkCreateBuffer(backendContext->device, &bufferInfo, nullptr, &testBuffer) != VK_SUCCESS)
            {
                FFX_ASSERT_FAIL("Cannot create test Breadcrumbs buffer to find memory requirements!");
                return FFX_ERROR_BACKEND_API_ERROR;
            }

            uint32_t memoryTypeBits = 0;
            /// Enable usage of dedicated memory for Breadcrumbs buffers only when is required by the implementation
            if (devCaps.dedicatedAllocationSupported)
            {
                if (!backendContext->vkFunctionTable.vkGetBufferMemoryRequirements2)
                {
                    backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, testBuffer, nullptr);
                    return FFX_ERROR_BACKEND_API_ERROR;
                }

                // Decide whether use dedicated memory or not
                VkBufferMemoryRequirementsInfo2 bufferReq = {};
                bufferReq.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
                bufferReq.pNext = nullptr;
                bufferReq.buffer = testBuffer;

                VkMemoryDedicatedRequirements dedicatedMemoryReq = {};
                dedicatedMemoryReq.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
                dedicatedMemoryReq.pNext = nullptr;
                dedicatedMemoryReq.requiresDedicatedAllocation = VK_FALSE;

                VkMemoryRequirements2 memoryReq2 = {};
                memoryReq2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
                memoryReq2.pNext = &dedicatedMemoryReq;

                backendContext->vkFunctionTable.vkGetBufferMemoryRequirements2(backendContext->device, &bufferReq, &memoryReq2);
                if (dedicatedMemoryReq.requiresDedicatedAllocation)
                    backendContext->breadcrumbsFlags |= BackendContext_VK::BREADCRUMBS_DEDICATED_MEMORY_ENABLED;
                memoryTypeBits = memoryReq2.memoryRequirements.memoryTypeBits;
            }
            else
            {
                VkMemoryRequirements memoryReq = {};
                backendContext->vkFunctionTable.vkGetBufferMemoryRequirements(backendContext->device, testBuffer, &memoryReq);
                memoryTypeBits = memoryReq.memoryTypeBits;
            }
            backendContext->vkFunctionTable.vkDestroyBuffer(backendContext->device, testBuffer, nullptr);

            // Find proper memory index for created buffers
            VkPhysicalDeviceMemoryProperties memoryProps = {};
            vkGetPhysicalDeviceMemoryProperties(backendContext->physicalDevice, &memoryProps);

            const VkMemoryPropertyFlags requiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            VkMemoryPropertyFlags preferredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            // When choosing between HOST_CACHED and AMD specific memory, AMD will take precedence as better guarantee of visible writes
            if (devCaps.deviceCoherentMemorySupported)
                preferredFlags |= VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD | VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD;

            backendContext->breadcrumbsMemoryIndex = UINT32_MAX;
            uint32_t memoryCost = UINT32_MAX;
            for (uint32_t i = 0, memoryBit = 1; i < memoryProps.memoryTypeCount; ++i, memoryBit <<= 1)
            {
                if (memoryTypeBits & memoryBit)
                {
                    const VkMemoryPropertyFlags memFlags = memoryProps.memoryTypes[i].propertyFlags;
                    if (!backendContext->deviceCoherentMemoryEnabled &&
                        (memFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD))
                        continue;
                    if ((memFlags & requiredMemoryFlags) == requiredMemoryFlags)
                    {
                        const uint32_t cost = ffxCountBitsSet(preferredFlags & ~memFlags);
                        if (cost < memoryCost)
                        {
                            backendContext->breadcrumbsMemoryIndex = i;
                            if (cost == 0)
                                break;
                            memoryCost = cost;
                        }
                    }
                }
            }

            if (backendContext->breadcrumbsMemoryIndex == UINT32_MAX)
            {
                FFX_ASSERT_FAIL("No memory that satisfies requirements requested by Breadcrumbs buffer type!");
                return FFX_ERROR_BACKEND_API_ERROR;
            }

            // Will switch to use vkCmdWriteBufferMarkerAMD() to write breadcrumbs into the buffer instead of vkCmdFillBuffer() for ensuring proper ordering of writes
            if (devCaps.bufferMarkerSupported)
                backendContext->breadcrumbsFlags |= BackendContext_VK::BREADCRUMBS_BUFFER_MARKER_ENABLED;

            // Together with BREADCRUMBS_BUFFER_MARKER_ENABLED flag will switch to vkCmdWriteBufferMarker2AMD() to use new synchronization facilities
            if (devCaps.extendedSynchronizationSupported)
                backendContext->breadcrumbsFlags |= BackendContext_VK::BREADCRUMBS_SYNCHRONIZATION2_ENABLED;
        }
    }

    // Increment the ref count
    ++backendContext->refCount;

    // Get an available context id
    for (uint32_t i = 0; i < backendContext->maxEffectContexts; ++i) {
        if (!backendContext->pEffectContexts[i].active) {
            *effectContextId = i;

            // Reset everything accordingly
            BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[i];
            effectContext.active = true;
            effectContext.effectId = effect;

            effectContext.nextStaticResource = (i * FFX_MAX_RESOURCE_COUNT) + 1;
            effectContext.nextDynamicResource = getDynamicResourcesStartIndex(i);
            effectContext.nextStaticResourceView = (i * FFX_MAX_QUEUED_FRAMES * FFX_MAX_RESOURCE_COUNT * 2);
            for (uint32_t frameIndex = 0; frameIndex < FFX_MAX_QUEUED_FRAMES; ++frameIndex)
            {
                effectContext.nextDynamicResourceView[frameIndex] = getDynamicResourceViewsStartIndex(i, frameIndex);
            }
            effectContext.nextPipelineLayout = (i * FFX_MAX_PASS_COUNT);
            effectContext.frameIndex = 0;

            if (bindlessConfig)
            {
                effectContext.bindlessTextureSrvHeapStart = backendContext->bindlessBase;
                effectContext.bindlessTextureSrvHeapSize  = bindlessConfig->maxTextureSrvs;

                backendContext->bindlessBase += bindlessConfig->maxTextureSrvs;

                effectContext.bindlessBufferSrvHeapSize  = bindlessConfig->maxBufferSrvs;

                effectContext.bindlessTextureUavHeapStart = backendContext->bindlessBase;
                effectContext.bindlessTextureUavHeapSize  = bindlessConfig->maxTextureUavs;

                backendContext->bindlessBase += bindlessConfig->maxTextureUavs;

                effectContext.bindlessBufferUavHeapSize  = bindlessConfig->maxBufferUavs;

                
                // create a bindless descriptor pool local to the current effect
                VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
                VkDescriptorPoolSize       poolSizes[3];

                uint32_t poolSizeCount = 0;

                if (bindlessConfig->maxTextureSrvs > 0)
                {
                    VkDescriptorPoolSize& poolSize = poolSizes[poolSizeCount++];

                    poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    poolSize.descriptorCount = bindlessConfig->maxTextureSrvs;
                }

                if (bindlessConfig->maxTextureUavs > 0)
                {
                    VkDescriptorPoolSize& poolSize = poolSizes[poolSizeCount++];

                    poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    poolSize.descriptorCount = bindlessConfig->maxTextureUavs;
                }

                if (bindlessConfig->maxBufferSrvs > 0 || bindlessConfig->maxBufferUavs > 0)
                {
                    VkDescriptorPoolSize& poolSize = poolSizes[poolSizeCount++];

                    poolSize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    poolSize.descriptorCount = bindlessConfig->maxBufferSrvs + bindlessConfig->maxBufferUavs;
                }

                descriptorPoolCreateInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                descriptorPoolCreateInfo.pNext         = nullptr;
                descriptorPoolCreateInfo.flags         = 0;
                descriptorPoolCreateInfo.poolSizeCount = poolSizeCount;
                descriptorPoolCreateInfo.pPoolSizes    = poolSizes;
                descriptorPoolCreateInfo.maxSets       = poolSizeCount;

                if (backendContext->vkFunctionTable.vkCreateDescriptorPool(
                        backendContext->device, &descriptorPoolCreateInfo, nullptr, &effectContext.bindlessDescriptorPool) != VK_SUCCESS)
                {
                    return FFX_ERROR_BACKEND_API_ERROR;
                }

                // create the descriptor layout for bindless texture srv buffers
                if (bindlessConfig->maxTextureSrvs > 0)
                {
                    VkDescriptorSetLayoutBinding binding = {
                        0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, bindlessConfig->maxTextureSrvs, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

                    VkDescriptorBindingFlags bindingFlags[] = {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

                    VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags = {};

                    setLayoutBindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                    setLayoutBindingFlags.bindingCount  = 1;
                    setLayoutBindingFlags.pBindingFlags = bindingFlags;

                    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    layoutInfo.pNext                           = &setLayoutBindingFlags;
                    layoutInfo.bindingCount                    = 1;
                    layoutInfo.pBindings                       = &binding;

                    if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(
                            backendContext->device, &layoutInfo, nullptr, &effectContext.bindlessTextureSrvDescriptorSetLayout) != VK_SUCCESS)
                    {
                     
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }

                    // allocate descriptor set
                    VkDescriptorSetAllocateInfo setAllocateInfo = {};
                    setAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    setAllocateInfo.descriptorPool              = effectContext.bindlessDescriptorPool;
                    setAllocateInfo.descriptorSetCount          = 1;
                    setAllocateInfo.pSetLayouts                 = &effectContext.bindlessTextureSrvDescriptorSetLayout;

                    backendContext->vkFunctionTable.vkAllocateDescriptorSets(
                        backendContext->device, &setAllocateInfo, &effectContext.bindlessTextureSrvDescriptorSet);
                }

                // create the descriptor layout for bindless buffer srv buffers
                if (bindlessConfig->maxBufferSrvs > 0)
                {
                    VkDescriptorSetLayoutBinding binding = {
                        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, bindlessConfig->maxBufferSrvs, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

                    VkDescriptorBindingFlags bindingFlags[] = {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

                    VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags = {};

                    setLayoutBindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                    setLayoutBindingFlags.bindingCount  = 1;
                    setLayoutBindingFlags.pBindingFlags = bindingFlags;

                    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    layoutInfo.pNext                           = &setLayoutBindingFlags;
                    layoutInfo.bindingCount                    = 1;
                    layoutInfo.pBindings                       = &binding;

                    if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(
                            backendContext->device, &layoutInfo, nullptr, &effectContext.bindlessBufferSrvDescriptorSetLayout) != VK_SUCCESS)
                    {
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }

                    // allocate descriptor set
                    VkDescriptorSetAllocateInfo setAllocateInfo = {};
                    setAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    setAllocateInfo.descriptorPool              = effectContext.bindlessDescriptorPool;
                    setAllocateInfo.descriptorSetCount          = 1;
                    setAllocateInfo.pSetLayouts                 = &effectContext.bindlessBufferSrvDescriptorSetLayout;

                    backendContext->vkFunctionTable.vkAllocateDescriptorSets(
                        backendContext->device, &setAllocateInfo, &effectContext.bindlessBufferSrvDescriptorSet);
                }

                // create the descriptor layout for bindless texture uav buffers
                if (bindlessConfig->maxTextureUavs > 0)
                {
                    VkDescriptorSetLayoutBinding binding = {
                        0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, bindlessConfig->maxTextureUavs, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

                    VkDescriptorBindingFlags bindingFlags[] = {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

                    VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags = {};

                    setLayoutBindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                    setLayoutBindingFlags.bindingCount  = 1;
                    setLayoutBindingFlags.pBindingFlags = bindingFlags;

                    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    layoutInfo.pNext                           = &setLayoutBindingFlags;
                    layoutInfo.bindingCount                    = 1;
                    layoutInfo.pBindings                       = &binding;

                    if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(
                            backendContext->device, &layoutInfo, nullptr, &effectContext.bindlessTextureUavDescriptorSetLayout) != VK_SUCCESS)
                    {
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }

                    // allocate descriptor set
                    VkDescriptorSetAllocateInfo setAllocateInfo = {};
                    setAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    setAllocateInfo.descriptorPool              = effectContext.bindlessDescriptorPool;
                    setAllocateInfo.descriptorSetCount          = 1;
                    setAllocateInfo.pSetLayouts                 = &effectContext.bindlessTextureUavDescriptorSetLayout;

                    backendContext->vkFunctionTable.vkAllocateDescriptorSets(
                        backendContext->device, &setAllocateInfo, &effectContext.bindlessTextureUavDescriptorSet);
                }

                // create the descriptor layout for bindless buffer uav buffers
                if (bindlessConfig->maxBufferUavs > 0)
                {
                    VkDescriptorSetLayoutBinding binding = {
                        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, bindlessConfig->maxBufferUavs, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

                    VkDescriptorBindingFlags bindingFlags[] = {VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

                    VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlags = {};

                    setLayoutBindingFlags.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                    setLayoutBindingFlags.bindingCount  = 1;
                    setLayoutBindingFlags.pBindingFlags = bindingFlags;

                    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
                    layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                    layoutInfo.pNext                           = &setLayoutBindingFlags;
                    layoutInfo.bindingCount                    = 1;
                    layoutInfo.pBindings                       = &binding;

                    if (backendContext->vkFunctionTable.vkCreateDescriptorSetLayout(
                            backendContext->device, &layoutInfo, nullptr, &effectContext.bindlessBufferUavDescriptorSetLayout) != VK_SUCCESS)
                    {
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }

                    // allocate descriptor set
                    VkDescriptorSetAllocateInfo setAllocateInfo = {};
                    setAllocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                    setAllocateInfo.descriptorPool              = effectContext.bindlessDescriptorPool;
                    setAllocateInfo.descriptorSetCount          = 1;
                    setAllocateInfo.pSetLayouts                 = &effectContext.bindlessBufferUavDescriptorSetLayout;

                    backendContext->vkFunctionTable.vkAllocateDescriptorSets(
                        backendContext->device, &setAllocateInfo, &effectContext.bindlessBufferUavDescriptorSet);
                }
            }
            else
            {
                effectContext.bindlessTextureSrvHeapStart = 0;
                effectContext.bindlessTextureSrvHeapSize  = 0;
                effectContext.bindlessBufferSrvHeapSize   = 0;
                effectContext.bindlessTextureUavHeapStart = 0;
                effectContext.bindlessTextureUavHeapSize  = 0;
                effectContext.bindlessBufferUavHeapSize   = 0;
            }   

            break;
        }
    }

    return FFX_OK;
}

FfxErrorCode GetDeviceCapabilitiesVK(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities)
{
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;

    // no shader model in vulkan so assume the minimum
    deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
    deviceCapabilities->waveLaneCountMin = 32;
    deviceCapabilities->waveLaneCountMax = 32;
    deviceCapabilities->fp16Supported = false;
    deviceCapabilities->raytracingSupported = false;
    deviceCapabilities->deviceCoherentMemorySupported = false;
    deviceCapabilities->dedicatedAllocationSupported = false;
    deviceCapabilities->bufferMarkerSupported = false;
    deviceCapabilities->extendedSynchronizationSupported = false;
    deviceCapabilities->shaderStorageBufferArrayNonUniformIndexing = false;

    BackendContext_VK* context = (BackendContext_VK*)backendInterface->scratchBuffer;

    // check if extensions are enabled

    for (uint32_t i = 0; i < backendContext->numDeviceExtensions; i++)
    {
        if (strcmp(backendContext->extensionProperties[i].extensionName, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME) == 0 &&
            backendContext->subgroupSizeControlEnabled)
        {
            // check if we the max subgroup size allows us to use wave64
            VkPhysicalDeviceSubgroupSizeControlProperties subgroupSizeControlProperties = {};
            subgroupSizeControlProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES;

            VkPhysicalDeviceProperties2 deviceProperties2 = {};
            deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            deviceProperties2.pNext = &subgroupSizeControlProperties;
            vkGetPhysicalDeviceProperties2(context->physicalDevice, &deviceProperties2);

            if (subgroupSizeControlProperties.requiredSubgroupSizeStages & VK_SHADER_STAGE_COMPUTE_BIT)
            {
                deviceCapabilities->waveLaneCountMin = subgroupSizeControlProperties.minSubgroupSize;
                deviceCapabilities->waveLaneCountMax = subgroupSizeControlProperties.maxSubgroupSize;
            }
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME) == 0 &&
                 backendContext->shaderFloat16Enabled)
        {
            // check for fp16 support
            VkPhysicalDeviceShaderFloat16Int8Features shaderFloat18Int8Features = {};
            shaderFloat18Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &shaderFloat18Int8Features;

            vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->fp16Supported = shaderFloat18Int8Features.shaderFloat16 == VK_TRUE;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0 &&
                 backendContext->accelerationStructureEnabled)
        {
            // check for ray tracing support
            VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
            accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &accelerationStructureFeatures;

            vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->raytracingSupported = accelerationStructureFeatures.accelerationStructure == VK_TRUE;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME) == 0)
        {
            // check for coherent memory support
            VkPhysicalDeviceCoherentMemoryFeaturesAMD coherentMemoryFeatures = {};
            coherentMemoryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &coherentMemoryFeatures;

            vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->deviceCoherentMemorySupported =
                backendContext->deviceCoherentMemoryEnabled == VK_TRUE &&
                coherentMemoryFeatures.deviceCoherentMemory == VK_TRUE;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_AMD_BUFFER_MARKER_EXTENSION_NAME) == 0 &&
                 backendContext->bufferMarkerEnabled)
        {
            // no features structure so extension name is enough
            deviceCapabilities->bufferMarkerSupported =
                backendContext->vkFunctionTable.vkCmdWriteBufferMarkerAMD != nullptr;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME) == 0 &&
                 backendContext->synchronization2Enabled)
        {
            // check for extended synchronization support
            VkPhysicalDeviceSynchronization2FeaturesKHR synchronizationFeatures = {};
            synchronizationFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &synchronizationFeatures;

            vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->extendedSynchronizationSupported =
                synchronizationFeatures.synchronization2 == VK_TRUE &&
                backendContext->vkFunctionTable.vkCmdWriteBufferMarker2AMD != nullptr;
        }
        else if (strcmp(backendContext->extensionProperties[i].extensionName, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0 &&
                 backendContext->shaderStorageBufferArrayNonUniformIndexingEnabled)
        {
            // check for coherent memory support
            VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {};
            descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;

            VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {};
            physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            physicalDeviceFeatures2.pNext = &descriptorIndexingFeatures;

            vkGetPhysicalDeviceFeatures2(context->physicalDevice, &physicalDeviceFeatures2);

            deviceCapabilities->shaderStorageBufferArrayNonUniformIndexing =
                descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing == VK_TRUE;
        }
    }

    // Dedicated allocation and memory-requirements2 were promoted to core in
    // Vulkan 1.1. Extension availability does not say whether the application
    // enabled the KHR command, so use the callable device entry point instead.
    deviceCapabilities->dedicatedAllocationSupported =
        backendContext->vkFunctionTable.vkGetBufferMemoryRequirements2 != nullptr;

    return FFX_OK;
}

FfxErrorCode DestroyBackendContextVK(FfxInterface* backendInterface, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_VK* backendContext = (BackendContext_VK*)backendInterface->scratchBuffer;
    FFX_ASSERT(backendContext->refCount > 0);

    // Delete any resources allocated by this context
    BackendContext_VK::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    for (uint32_t currentStaticResourceIndex = effectContextId * FFX_MAX_RESOURCE_COUNT; currentStaticResourceIndex < effectContext.nextStaticResource; ++currentStaticResourceIndex) {

        if (backendContext->pResources[currentStaticResourceIndex].imageResource != VK_NULL_HANDLE) {
            FFX_ASSERT_MESSAGE(false, "FFXInterface: Vulkan: SDK Resource was not destroyed prior to destroying the backend context. There is a resource leak.");
            FfxResourceInternal internalResource = { static_cast<int32_t>(currentStaticResourceIndex) };
            DestroyResourceVK(backendInterface, internalResource, effectContextId);
        }
    }

    for (uint32_t frameIndex = 0; frameIndex < FFX_MAX_QUEUED_FRAMES; ++frameIndex)
        destroyDynamicViews(backendContext, effectContextId, frameIndex);

    // clean up descriptor set layouts
    if (effectContext.bindlessTextureSrvDescriptorSetLayout)
    {
        backendContext->vkFunctionTable.vkFreeDescriptorSets(backendContext->device, effectContext.bindlessDescriptorPool, 1, &effectContext.bindlessTextureSrvDescriptorSet);
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(
            backendContext->device, effectContext.bindlessTextureSrvDescriptorSetLayout, VK_NULL_HANDLE);
        effectContext.bindlessTextureSrvDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (effectContext.bindlessBufferSrvDescriptorSetLayout)
    {
        backendContext->vkFunctionTable.vkFreeDescriptorSets(backendContext->device, effectContext.bindlessDescriptorPool, 1, &effectContext.bindlessBufferSrvDescriptorSet);
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(
            backendContext->device, effectContext.bindlessBufferSrvDescriptorSetLayout, VK_NULL_HANDLE);
        effectContext.bindlessBufferSrvDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (effectContext.bindlessTextureUavDescriptorSetLayout)
    {
        backendContext->vkFunctionTable.vkFreeDescriptorSets(backendContext->device, effectContext.bindlessDescriptorPool, 1, &effectContext.bindlessTextureUavDescriptorSet);
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(
            backendContext->device, effectContext.bindlessTextureUavDescriptorSetLayout, VK_NULL_HANDLE);
        effectContext.bindlessTextureUavDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (effectContext.bindlessBufferUavDescriptorSetLayout)
    {
        backendContext->vkFunctionTable.vkFreeDescriptorSets(backendContext->device, effectContext.bindlessDescriptorPool, 1, &effectContext.bindlessBufferUavDescriptorSet);
        backendContext->vkFunctionTable.vkDestroyDescriptorSetLayout(
            backendContext->device, effectContext.bindlessBufferUavDescriptorSetLayout, VK_NULL_HANDLE);
        effectContext.bindlessBufferUavDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (effectContext.bindlessDescriptorPool)
    {
        backendContext->vkFunctionTable.vkDestroyDescriptorPool(backendContext->device, effectContext.bindlessDescriptorPool, VK_NULL_HANDLE);
        effectContext.bindlessTextureSrvDescriptorSetLayout = VK_NULL_HANDLE;
    }

    // Free up for use by another context
    effectContext.nextStaticResource = 0;
    effectContext.active = false;

    // Decrement ref count
    --backendContext->refCount;

    if (!backendContext->refCount) {

        // clean up descriptor pool
