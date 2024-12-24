#pragma once

#include "../../pch.hpp"

#include "../Config.hpp"
#include "Renderpass.hpp"

struct DescriptorSetLayoutData {
    uint32_t set_number;
    VkDescriptorSetLayoutCreateInfo create_info;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

struct ShaderModuleReflectionData {
    bool hasVertexBindingDescription;
    VkVertexInputBindingDescription inputBindingDescription;
    std::vector<VkVertexInputAttributeDescription> inputDescriptions;
    std::vector<VkPushConstantRange> pushConstants;
    std::vector<DescriptorSetLayoutData> descriptorSets;
};

struct ShaderModule {
    VkDevice device;

    VkShaderModule shaderModule;

    VkPipelineShaderStageCreateInfo shaderStageInfo;

    ShaderModuleReflectionData reflectionData;

    ShaderModule(VkDevice device, VkShaderModule shaderModule,
                 VkPipelineShaderStageCreateInfo shaderStageInfo,
                 ShaderModuleReflectionData reflectionData);

    ~ShaderModule();
};

struct PipelineCreateInfo {
    const char* vertexShaderPath;
    const char* fragmentShaderPath;
    uint32_t viewportWidth;
    uint32_t viewportHeight;
    bool culling;
    bool depthTesting;
    bool depthWrite;
    std::shared_ptr<RenderPass> renderPass;
};

struct Pipeline { // TODO: Support descriptor sets, push constants, etc.
    VkDevice device;

    VkPipeline pipeline;

    VkPipelineLayout layout;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

    Pipeline(VkDevice device, VkPipeline pipeline, VkPipelineLayout layout,
             std::vector<VkDescriptorSetLayout> descriptorSetLayouts);

    ~Pipeline();
};

enum class DescriptorType { UNIFORM_BUFFER, STORAGE_BUFFER };

struct DescriptorSet {
    VmaAllocator allocator;

    std::array<VkDescriptorSet, FRAME_OVERLAP> descriptorSets;
    VkPipelineLayout pipelineLayout;

    std::array<std::map<unsigned int, VkBuffer>, FRAME_OVERLAP> buffers;
    std::array<std::map<unsigned int, VmaAllocation>, FRAME_OVERLAP> allocations;

    DescriptorSet(VmaAllocator allocator, std::array<VkDescriptorSet, FRAME_OVERLAP> descriptorSets,
                  VkPipelineLayout pipelineLayout);

    ~DescriptorSet();
};
