#include "VulkanRenderer.h"
#include "Actor.h"

void VulkanRenderer::colorPickBindImageInfo()
{
    VkImageCreateInfo colorPickerImageInfo{};
    colorPickerImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    colorPickerImageInfo.imageType = VK_IMAGE_TYPE_2D;
    colorPickerImageInfo.extent.width = swapChainExtent.width;
    colorPickerImageInfo.extent.height = swapChainExtent.height;
    colorPickerImageInfo.extent.depth = 1;
    colorPickerImageInfo.mipLevels = 1;
    colorPickerImageInfo.arrayLayers = 1;
    colorPickerImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorPickerImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    colorPickerImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorPickerImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    colorPickerImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    colorPickerImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &colorPickerImageInfo, nullptr, &colorPickerImage) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, colorPickerImage, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &pickImageMem) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }
    vkBindImageMemory(device, colorPickerImage, pickImageMem, 0);
}

void VulkanRenderer::createStagingBuffer(BufferMemory& buffer)
{
    VkDeviceSize imageSize = swapChainExtent.width * swapChainExtent.height * 4;

    CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer.bufferID, buffer.bufferMemoryID);
}

void VulkanRenderer::createOffScreenImageView()
{
    colorPickerImageView = createImageView(colorPickerImage, colorPickerImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanRenderer::createColorPickerRenderPass() {
    //VkAttachment description -> describes the attachment of a render pass.
    VkAttachmentDescription colorAttachment{};
    //For color picking, the format must store the color IDs.
    colorAttachment.format = colorPickerImageFormat;
    //This means that the buffer is 1x sampled because we want the exact color under the cursor.
    //we want 1 sample per pixel
    //sample->a piece of data representing either a color, depth or result of shading
    //pixel can hold multiple samples. 
    //pixels are justt our screen, the samples actually whatever we put on it. so that very section
    //of the pixels -> holds the sample.
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    //Clear the old pick colors 
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //the pick colors are stored which can be used in vkCmdCopyImageToBuffer, but rn its the reverse what
    //we aare doing for color picking
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //NO STENCIL OR DEPTH IN THIS CASE
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //whatever the old contained image was, we don't care about that
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //***VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL -> transitions the image into a source layout
    //we are using the layout in copying image to buffer. 
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    //whatever color we are passing in the shader, this attachment is where the colors should be drawn
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


    ///Subpass is the actual stage where the drawing happens
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    //Before Subpass 0 begins writing color data,
    //wait until all external color - output operations are done,
    //then allow Subpass 0 to write to the color attachment.
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    //std::array<VkAttachmentDescription, 1> attachments = { colorAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    //then you create a render pass
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &colorPickerRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void VulkanRenderer::createColorPickerFrameBuffer() {

    //A framebuffer connects your render pass to the actual image that your pipeline will draw into
    //so, the render pass holds a layout, format for an image, frambuffer holds the actual imageView itself.
    //this attachment is the actual imageView
    VkImageView attachment = colorPickerImageView;

    VkFramebufferCreateInfo colorPickerFramebufferInfo{};
    colorPickerFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    colorPickerFramebufferInfo.renderPass = colorPickerRenderPass;
    colorPickerFramebufferInfo.attachmentCount = 1;
    colorPickerFramebufferInfo.pAttachments = &attachment;
    //So the swapChains are still rendering the main scene, this color picker frame buffer should also 
    // be the same size as of that of swapchains.
    //Hardcoded the width and height
    //the image would be of this width and height 
    colorPickerFramebufferInfo.width = swapChainExtent.width;
    colorPickerFramebufferInfo.height = swapChainExtent.height;
    colorPickerFramebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &colorPickerFramebufferInfo, nullptr, &colorPickerFrameBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

void VulkanRenderer::createColorPickerCommandPool() {
    //The command pool is the memory manager for the allocating the command buffers. 
    //pool for recording the commands related to color-picker render pass
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &colorPickerCommandBufferData.colorPickerCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics command pool!");
    }


}

void VulkanRenderer::createColorPickerCommandBuffer 
()
{
    //Command Buffer is where we record our GPU commands.
    //we don't really call the draw functions directly, we record them into a command buffer and submit in a queue
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = colorPickerCommandBufferData.colorPickerCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    //this cmd allows us to record rendering commands into it.
    if (vkAllocateCommandBuffers(device, &allocInfo, &colorPickerCommandBufferData.colorPickerCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }


}

void VulkanRenderer::copyImagetoBuffer()
{

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { swapChainExtent.width, swapChainExtent.height, 1 };


    vkCmdCopyImageToBuffer(colorPickerCommandBufferData.colorPickerCommandBuffer, colorPickerImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.bufferID, 1, &region);

}

void VulkanRenderer::RecordCPCommandBuffer(Recording start_stop) {
    if (start_stop == Recording::START) {

        vkDeviceWaitIdle(device);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(colorPickerCommandBufferData.colorPickerCommandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording color picker command buffer!");
        }

        VkRenderPassBeginInfo cbRenderPassInfo{};
        cbRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        cbRenderPassInfo.renderPass = colorPickerRenderPass;
        cbRenderPassInfo.framebuffer = colorPickerFrameBuffer;
        cbRenderPassInfo.renderArea.offset = { 0, 0 };
        cbRenderPassInfo.renderArea.extent = swapChainExtent;


        VkClearValue clearValue;
        ///CHANGE THE BACKGROUND COLOR
        clearValue.color = { 1.0f, 1.0f, 1.0f, 1.0f };


        cbRenderPassInfo.clearValueCount = 1;
        cbRenderPassInfo.pClearValues = &clearValue;
        vkCmdBeginRenderPass(colorPickerCommandBufferData.colorPickerCommandBuffer, &cbRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
    //stop the recording
    //copy the image to buffer
    //suubmit the queue
    else if (start_stop == Recording::STOP)
    {
        vkCmdEndRenderPass(colorPickerCommandBufferData.colorPickerCommandBuffer);
        //copy from the frame buffer to the staging buffer
        copyImagetoBuffer();
        if (vkEndCommandBuffer(colorPickerCommandBufferData.colorPickerCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record color picker command buffer!");
        }
        ///submitting the buffer to be executed
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &colorPickerCommandBufferData.colorPickerCommandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

    }


}

void VulkanRenderer::DestroyCPCommandBuffers() {

    vkFreeCommandBuffers(device, colorPickerCommandBufferData.colorPickerCommandPool, 1, &colorPickerCommandBufferData.colorPickerCommandBuffer);
    vkDestroyCommandPool(device, colorPickerCommandBufferData.colorPickerCommandPool, nullptr);
}

void VulkanRenderer::BindColorPickerPipeline(VkPipeline pipeline)
{
    vkCmdBindPipeline(colorPickerCommandBufferData.colorPickerCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

}

void VulkanRenderer::BindCPDescriptorSet(VkPipelineLayout pipelineLayout, VkDescriptorSet descriptorSet, uint32_t descriptorIndex)
{
    vkCmdBindDescriptorSets(colorPickerCommandBufferData.colorPickerCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout, descriptorIndex, 1, &descriptorSet, 0, nullptr);


}

void VulkanRenderer::BindMeshCP(IndexedVertexBuffer mesh_) {
    VkBuffer vertexBuffers[] = { mesh_.vertBufferID };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(colorPickerCommandBufferData.colorPickerCommandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(colorPickerCommandBufferData.colorPickerCommandBuffer, mesh_.indexBufferID, 0, VK_INDEX_TYPE_UINT32);

}

void VulkanRenderer::DrawIndexedCP(IndexedVertexBuffer mesh_) {

    vkCmdDrawIndexed(colorPickerCommandBufferData.colorPickerCommandBuffer, static_cast<uint32_t>(mesh_.indexBufferLength), 1, 0, 0, 0);

}

uint32_t VulkanRenderer::extractColorID(int x_, int y_)
{
    //using struct to align the memory acc to image format sent by gpu
    struct pixel {
        uint8_t R;
        uint8_t G;
        uint8_t B;
        uint8_t A;
    };


    vkDeviceWaitIdle(device);
    //create the function which creates THAT(SCOTT'S EXPLANATION) pointer, 
    //which reads the data back from gpu to cpu 
    void* data;
    //vkMapMemory which is in VulkanSampler for ref
    vkMapMemory(device, stagingBuffer.bufferMemoryID, 0, stagingBuffer.bufferMemoryLength, 0, &data);
    //we cast the *data to the *pixel 
    pixel* pixelbuffer = reinterpret_cast<pixel*>(data);
    //we are conveerting a 2d-array to 1d array. pixel coordinates are 2d array and int index is 1d
    int index = swapChainExtent.width * y_ + x_;

    //converrt those values into uint32 to combine them as one color id
    uint32_t r = (uint32_t)pixelbuffer[index].R;
    uint32_t g = (uint32_t)pixelbuffer[index].G;
    //bit wise shifting 8 bits
    g = g << 8;
    uint32_t b = (uint32_t)pixelbuffer[index].B;
    //bit wise shifting 16 bits
    b = b << 16;

    //colorid = R|G|B combining
    uint32_t colorID = r | b | g;
    vkUnmapMemory(device, stagingBuffer.bufferMemoryID);
    return colorID;
}

void VulkanRenderer::SetPushConstantCP(const ColorPickerPipelineInfo pipelineInfo, Actor* actor) {

    ModelMatrixPushConst modelMatrixPushConst;
    modelMatrixPushConst.modelMatrix = actor->getModelMatrix();
    modelMatrixPushConst.pad = actor->getColorID();
    Matrix3 normalMatrix = Matrix3(MMath::transpose(MMath::inverse(actor->getModelMatrix())));

    /// See the header file for an explanation of how I laid out this out in memory
    modelMatrixPushConst.normalMatrix[0].x = normalMatrix[0];
    modelMatrixPushConst.normalMatrix[1].x = normalMatrix[1];
    modelMatrixPushConst.normalMatrix[2].x = normalMatrix[2];

    modelMatrixPushConst.normalMatrix[0].y = normalMatrix[3];
    modelMatrixPushConst.normalMatrix[1].y = normalMatrix[4];
    modelMatrixPushConst.normalMatrix[2].y = normalMatrix[5];

    modelMatrixPushConst.normalMatrix[0].z = normalMatrix[6];
    modelMatrixPushConst.normalMatrix[1].z = normalMatrix[7];
    modelMatrixPushConst.normalMatrix[2].z = normalMatrix[8];


    vkCmdPushConstants(colorPickerCommandBufferData.colorPickerCommandBuffer, pipelineInfo.colorPickerPipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ModelMatrixPushConst), &modelMatrixPushConst);

}

void VulkanRenderer::DestroyCPPipeline(ColorPickerPipelineInfo pipelineInfo) {
    vkDestroyPipelineLayout(device, pipelineInfo.colorPickerPipelineLayout, nullptr);
    vkDestroyPipeline(device, pipelineInfo.colorPickerPipeline, nullptr);
}

void VulkanRenderer::DestroyCPDescriptorSet(DescriptorSetInfo& descriptorSetInfo) {
    vkDestroyDescriptorSetLayout(device, descriptorSetInfo.descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorSetInfo.descriptorPool, nullptr);
    descriptorSetInfo.descriptorSet.clear();
}

ColorPickerPipelineInfo VulkanRenderer::createColorPickerPipeline(VkDescriptorSetLayout descriptorSetLayout, const char* vertFile, const char* fragFile,
    const char* tessCtrlFile, const char* tessEvalFile, const char* geomFile, const char* colorPickingFile)
{

    ColorPickerPipelineInfo colorPickerPipelineInfo;
    ///descriptor - handle that connects VRAM with the processing unit
    ///descriptorset - bunch of descriptors


    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    std::vector<char> tessCtrlCode;
    std::vector<char> tessEvalCode;
    std::vector<char> geomCode;


    VkShaderModule vertShaderModule = VK_NULL_HANDLE; //0ULL - Zero unsigned long long. 
    VkShaderModule fragShaderModule = VK_NULL_HANDLE;
    VkShaderModule tessCtrlModule = VK_NULL_HANDLE;
    VkShaderModule tessEvalModule = VK_NULL_HANDLE;
    VkShaderModule geomModule = VK_NULL_HANDLE;



    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    VkPipelineShaderStageCreateInfo tessCtrlStageInfo{};
    VkPipelineShaderStageCreateInfo tessEvalStageInfo{};
    VkPipelineShaderStageCreateInfo geomStageInfo{};



    if (vertFile != nullptr && fragFile != nullptr) {
        vertShaderCode = readFile(vertFile);
        fragShaderCode = readFile(fragFile);
        vertShaderModule = createShaderModule(vertShaderCode);
        fragShaderModule = createShaderModule(fragShaderCode);
    }

    if (tessCtrlFile != nullptr && tessEvalFile != nullptr) {
        tessCtrlCode = readFile(tessCtrlFile);
        tessEvalCode = readFile(tessEvalFile);
        tessCtrlModule = createShaderModule(tessCtrlCode);
        tessEvalModule = createShaderModule(tessEvalCode);
    }

    if (geomFile != nullptr) {
        geomCode = readFile(geomFile);
        geomModule = createShaderModule(geomCode);

    }




    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    tessCtrlStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    tessCtrlStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    tessCtrlStageInfo.module = tessCtrlModule;
    tessCtrlStageInfo.pName = "main";

    tessEvalStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    tessEvalStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    tessEvalStageInfo.module = tessEvalModule;
    tessEvalStageInfo.pName = "main";

    geomStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    geomStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    geomStageInfo.module = geomModule;
    geomStageInfo.pName = "main";





    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;


    if (vertShaderModule != VK_NULL_HANDLE) shaderStages.push_back(vertShaderStageInfo);
    if (tessCtrlModule != VK_NULL_HANDLE) shaderStages.push_back(tessCtrlStageInfo);
    if (tessEvalModule != VK_NULL_HANDLE) shaderStages.push_back(tessEvalStageInfo);
    if (geomModule != VK_NULL_HANDLE) shaderStages.push_back(geomStageInfo);
    if (fragShaderModule != VK_NULL_HANDLE) shaderStages.push_back(fragShaderStageInfo);




    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    //inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    //inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPushConstantRange range{};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    range.offset = 0;
    range.size = sizeof(ModelMatrixPushConst);

    //VkPushConstantRange range = {};
    //range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;  //change it to one after 
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &range;
    //pipelineLayoutInfo.flags = VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT;


    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &colorPickerPipelineInfo.colorPickerPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }



    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = colorPickerPipelineInfo.colorPickerPipelineLayout;
    pipelineInfo.renderPass = colorPickerRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
        nullptr, &colorPickerPipelineInfo.colorPickerPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }


    if (fragShaderModule) vkDestroyShaderModule(device, fragShaderModule, nullptr);
    if (fragShaderModule) vkDestroyShaderModule(device, vertShaderModule, nullptr);
    if (tessCtrlModule) vkDestroyShaderModule(device, tessCtrlModule, nullptr);
    if (tessEvalModule) vkDestroyShaderModule(device, tessEvalModule, nullptr);
    if (geomModule) vkDestroyShaderModule(device, geomModule, nullptr);

    return colorPickerPipelineInfo;
}

void VulkanRenderer::destroyColorPickerInfo()
{

    vkDestroyFramebuffer(device, colorPickerFrameBuffer, nullptr);
    vkDestroyImageView(device, colorPickerImageView, nullptr);
    vkDestroyImage(device, colorPickerImage, nullptr);
    vkFreeMemory(device, pickImageMem, nullptr);
    vkDestroyRenderPass(device, colorPickerRenderPass, nullptr);
    vkDestroyBuffer(device, stagingBuffer.bufferID, nullptr);
    vkFreeMemory(device, stagingBuffer.bufferMemoryID, nullptr);
    DestroyCPCommandBuffers();

}



