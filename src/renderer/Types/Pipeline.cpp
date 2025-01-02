#include "../../pch.hpp"
#include "Pipeline.hpp"

#include "../../Logger.hpp"

ShaderModule::ShaderModule(VkDevice device, VkShaderModule shaderModule,
                           VkPipelineShaderStageCreateInfo shaderStageInfo,
                           ShaderModuleReflectionData reflectionData)
    : device(device), shaderModule(shaderModule), shaderStageInfo(shaderStageInfo),
      reflectionData(reflectionData) {}

ShaderModule::~ShaderModule() {
    Logger::renderer_logger->info("Destroying Shader Module");

    if (device != VK_NULL_HANDLE) {
        if (shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, shaderModule, nullptr);
        }
    }
}

Pipeline::Pipeline(VkDevice device, VkPipeline pipeline, VkPipelineLayout layout,
                   std::vector<VkDescriptorSetLayout> descriptorSetLayouts)
    : device(device), pipeline(pipeline), layout(layout),
      descriptorSetLayouts(descriptorSetLayouts) {}

Pipeline::~Pipeline() {
    Logger::renderer_logger->info("Destroying Pipeline");

    if (device != VK_NULL_HANDLE) {
        for (auto& setLayout : descriptorSetLayouts) {
            vkDestroyDescriptorSetLayout(device, setLayout, nullptr);
        }
        if (layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, layout, nullptr);
        }
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
    }
}

DescriptorSet::DescriptorSet(VmaAllocator allocator,
                             std::array<VkDescriptorSet, FRAME_OVERLAP> descriptorSets,
                             VkPipelineLayout pipelineLayout)
    : allocator(allocator), descriptorSets(descriptorSets), pipelineLayout(pipelineLayout) {}

DescriptorSet::~DescriptorSet() {
    Logger::renderer_logger->info("Destroying Descriptor Set");

    if (allocator != VK_NULL_HANDLE) {
        for (int i = 0; i < FRAME_OVERLAP; i++) {
            for (auto it = buffers[i].begin(); it != buffers[i].end(); it++) {
                vmaDestroyBuffer(allocator, it->second, allocations[i][it->first]);
            }
        }
    }
}
