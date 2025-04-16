#include "PBRRenderer.hpp"

#include <stb_image.h>

#include "../Logger.hpp"
#include "../Structures/Mesh/mesh.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

PBRRenderer::PBRRenderer(std::shared_ptr<Window> window) {
    this->window = window;

    graphicsContext = GraphicsContext::create(window);

    renderFence      = graphicsContext->createFrameBasedFence(true);
    renderSemaphore  = graphicsContext->createFrameBasedSemaphore();
    presentSemaphore = graphicsContext->createFrameBasedSemaphore();

    mainCommandBuffer = graphicsContext->createFrameBasedCommandBuffer();

    // Create the environment and irradiance map
    environmentMap = graphicsContext->createCubemap(Format::RGBA16_FLOAT, 512, 512);
    irradianceMap  = graphicsContext->createCubemap(Format::RGBA16_FLOAT, 32, 32);
    prefilterMap   = graphicsContext->createCubemap(Format::RGBA16_FLOAT, 128, 128, true);

    processEnvironmentAndIrradianceMaps();

    // Create the BRDF Image
    std::vector<RenderPassAttachmentDescription> brdfAttachments;
    RenderPassAttachmentDescription brdfAttachment = {};
    brdfAttachment.loadOp                          = LoadOp::CLEAR;
    brdfAttachment.storeOp                         = StoreOp::STORE;
    brdfAttachment.initialLayout                   = ImageLayout::UNDEFINED;
    brdfAttachment.finalLayout                     = ImageLayout::ATTACHMENT;
    brdfAttachment.format                          = Format::RG16_FLOAT;
    brdfAttachment.width                           = 512;
    brdfAttachment.height                          = 512;
    brdfAttachments.push_back(brdfAttachment);
    auto brdfRenderPass = graphicsContext->createRenderPass(brdfAttachments, false);

    processBRDF(brdfRenderPass);
}

PBRRenderer::~PBRRenderer() { Logger::renderer_logger->info("PBRRenderer destroyed"); }

void PBRRenderer::processEnvironmentAndIrradianceMaps() {
    int width, height, numComp;
    float* hdrData  = stbi_loadf("assets/textures/night_stars.hdr", &width, &height, &numComp, 4);
    auto hdrTexture = graphicsContext->createHDRTexture(width, height, 4, hdrData, false);
    stbi_image_free(hdrData);

    std::vector<RenderPassAttachmentDescription> equiToCubeAttachments;
    RenderPassAttachmentDescription equiToCubeAttachment = {};
    equiToCubeAttachment.loadOp                          = LoadOp::CLEAR;
    equiToCubeAttachment.storeOp                         = StoreOp::STORE;
    equiToCubeAttachment.initialLayout                   = ImageLayout::UNDEFINED;
    equiToCubeAttachment.finalLayout                     = ImageLayout::ATTACHMENT;
    equiToCubeAttachment.format                          = Format::RGBA16_FLOAT;
    equiToCubeAttachment.width                           = 512;
    equiToCubeAttachment.height                          = 512;
    equiToCubeAttachments.push_back(equiToCubeAttachment);
    auto equiToCubeRenderPass = graphicsContext->createRenderPass(equiToCubeAttachments, false);

    PipelineCreateInfo equiToCubePipelineCreateInfo = {};
    equiToCubePipelineCreateInfo.vertexShaderPath   = "assets/shaders/equiToCube.vert";
    equiToCubePipelineCreateInfo.fragmentShaderPath = "assets/shaders/equiToCube.frag";
    equiToCubePipelineCreateInfo.viewportWidth      = 512;
    equiToCubePipelineCreateInfo.viewportHeight     = 512;
    equiToCubePipelineCreateInfo.culling            = false;
    equiToCubePipelineCreateInfo.depthTesting       = false;
    equiToCubePipelineCreateInfo.depthWrite         = true;
    equiToCubePipelineCreateInfo.renderPass         = equiToCubeRenderPass;
    auto equiToCubePipeline = graphicsContext->createPipeline(&equiToCubePipelineCreateInfo);

    auto equiTextureSet = graphicsContext->createDescriptorSet(equiToCubePipeline, 0);
    graphicsContext->descriptorSetAddImage(equiTextureSet, 0, hdrTexture);

    Mesh cubeMesh                    = Mesh::loadFromObj("assets/models/cube.obj");
    std::vector<Vertex> cubeVertices = std::vector<Vertex>();
    for (auto vertex : cubeMesh.vertices) {
        cubeVertices.push_back({ vertex.position, vertex.normal, vertex.uv });
    }

    auto cubeVertexBuffer = graphicsContext->createVertexBuffer(
        cubeVertices.data(), uint32_t(cubeVertices.size() * sizeof(Vertex)));

    auto equiToCubeCommandBuffer = graphicsContext->createCommandBuffer();

    // Environment Map
    graphicsContext->beginRecording(equiToCubeCommandBuffer);

    for (int i = 0; i < 6; i++) {
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        captureProjection[1][1] *= -1;
        glm::mat4 viewMatrix(1.0f);
        switch (i) {
        case 0: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        }
        case 1: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        }
        case 2: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                                     glm::vec3(0.0f, 0.0f, -1.0f));

            break;
        }
        case 3: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                                     glm::vec3(0.0f, 0.0f, 1.0f));
            break;
        }
        case 4: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                                     glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        }
        case 5: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                     glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        }
        }
        viewMatrix = captureProjection * viewMatrix;

        graphicsContext->beginRenderPass(equiToCubeCommandBuffer, equiToCubeRenderPass, 512, 512);
        graphicsContext->bindPipeline(equiToCubeCommandBuffer, equiToCubePipeline);
        graphicsContext->bindVertexBuffer(equiToCubeCommandBuffer, cubeVertexBuffer);
        graphicsContext->pushConstants(equiToCubeCommandBuffer, equiToCubePipeline, 0,
                                       sizeof(glm::mat4), &viewMatrix);
        graphicsContext->bindDescriptorSet(equiToCubeCommandBuffer, 0, equiTextureSet);
        graphicsContext->draw(equiToCubeCommandBuffer, (uint32_t)cubeVertices.size(), 1, 0, 0);
        graphicsContext->endRenderPass(equiToCubeCommandBuffer);

        graphicsContext->transitionRenderPassImages(equiToCubeCommandBuffer, equiToCubeRenderPass,
                                                    ImageLayout::ATTACHMENT,
                                                    ImageLayout::TRANSFER_SRC);

        graphicsContext->copyRenderPassImageToCubemap(equiToCubeCommandBuffer, equiToCubeRenderPass,
                                                      0, environmentMap, i, 0, 512, 512);
    }

    graphicsContext->endRecording(equiToCubeCommandBuffer);
    graphicsContext->immediateSubmit(equiToCubeCommandBuffer);

    // Irradiance Map
    std::vector<RenderPassAttachmentDescription> convolutionRenderPassAttachments;
    RenderPassAttachmentDescription convolutionMainAttachment = {};
    convolutionMainAttachment.loadOp                          = LoadOp::CLEAR;
    convolutionMainAttachment.storeOp                         = StoreOp::STORE;
    convolutionMainAttachment.initialLayout                   = ImageLayout::UNDEFINED;
    convolutionMainAttachment.finalLayout                     = ImageLayout::ATTACHMENT;
    convolutionMainAttachment.format                          = Format::RGBA16_FLOAT;
    convolutionMainAttachment.width                           = 32;
    convolutionMainAttachment.height                          = 32;
    convolutionRenderPassAttachments.push_back(convolutionMainAttachment);
    auto convolutionRenderPass =
        graphicsContext->createRenderPass(convolutionRenderPassAttachments, false);

    PipelineCreateInfo convolutionPipelineCreateInfo = {};
    convolutionPipelineCreateInfo.vertexShaderPath   = "assets/shaders/convolution.vert";
    convolutionPipelineCreateInfo.fragmentShaderPath = "assets/shaders/convolution.frag";
    convolutionPipelineCreateInfo.viewportWidth      = 32;
    convolutionPipelineCreateInfo.viewportHeight     = 32;
    convolutionPipelineCreateInfo.culling            = false;
    convolutionPipelineCreateInfo.depthTesting       = true;
    convolutionPipelineCreateInfo.depthWrite         = true;
    convolutionPipelineCreateInfo.renderPass         = convolutionRenderPass;
    auto convolutionPipeline = graphicsContext->createPipeline(&convolutionPipelineCreateInfo);

    auto environmentMapDescriptorSet = graphicsContext->createDescriptorSet(convolutionPipeline, 0);
    graphicsContext->descriptorSetAddImage(environmentMapDescriptorSet, 0, environmentMap);

    graphicsContext->beginRecording(equiToCubeCommandBuffer);

    for (int i = 0; i < 6; i++) {
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 viewMatrix(1.0f);
        switch (i) {
        case 0: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        }
        case 1: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        }
        case 2: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                                     glm::vec3(0.0f, 0.0f, 1.0f));
            break;
        }
        case 3: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                                     glm::vec3(0.0f, 0.0f, -1.0f));
            break;
        }
        case 4: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                                     glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        }
        case 5: {
            viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                     glm::vec3(0.0f, -1.0f, 0.0f));
            break;
        }
        }

        viewMatrix = captureProjection * viewMatrix;

        graphicsContext->beginRenderPass(equiToCubeCommandBuffer, convolutionRenderPass, 32, 32);
        graphicsContext->bindPipeline(equiToCubeCommandBuffer, convolutionPipeline);
        graphicsContext->bindDescriptorSet(equiToCubeCommandBuffer, 0, environmentMapDescriptorSet);
        graphicsContext->pushConstants(equiToCubeCommandBuffer, convolutionPipeline, 0,
                                       sizeof(glm::mat4), &viewMatrix);
        graphicsContext->bindVertexBuffer(equiToCubeCommandBuffer, cubeVertexBuffer);

        graphicsContext->draw(equiToCubeCommandBuffer, (uint32_t)cubeVertices.size(), 1, 0, 0);

        graphicsContext->endRenderPass(equiToCubeCommandBuffer);

        graphicsContext->transitionRenderPassImages(equiToCubeCommandBuffer, convolutionRenderPass,
                                                    ImageLayout::ATTACHMENT,
                                                    ImageLayout::TRANSFER_SRC);

        graphicsContext->copyRenderPassImageToCubemap(
            equiToCubeCommandBuffer, convolutionRenderPass, 0, irradianceMap, i, 0, 32, 32);
    }

    graphicsContext->endRecording(equiToCubeCommandBuffer);
    graphicsContext->immediateSubmit(equiToCubeCommandBuffer);

    // Prefilter Map
    std::vector<RenderPassAttachmentDescription> prefilterAttachments;
    RenderPassAttachmentDescription prefilterAttachment = {};
    prefilterAttachment.loadOp                          = LoadOp::CLEAR;
    prefilterAttachment.storeOp                         = StoreOp::STORE;
    prefilterAttachment.initialLayout                   = ImageLayout::UNDEFINED;
    prefilterAttachment.finalLayout                     = ImageLayout::ATTACHMENT;
    prefilterAttachment.format                          = Format::RGBA16_FLOAT;
    prefilterAttachment.width                           = 128;
    prefilterAttachment.height                          = 128;
    prefilterAttachments.push_back(prefilterAttachment);
    auto prefilterRenderPass = graphicsContext->createRenderPass(prefilterAttachments, false);

    PipelineCreateInfo prefilterPipelineCreateInfo = {};
    prefilterPipelineCreateInfo.vertexShaderPath   = "assets/shaders/prefilter.vert";
    prefilterPipelineCreateInfo.fragmentShaderPath = "assets/shaders/prefilter.frag";
    prefilterPipelineCreateInfo.viewportWidth      = 128;
    prefilterPipelineCreateInfo.viewportHeight     = 128;
    prefilterPipelineCreateInfo.culling            = false;
    prefilterPipelineCreateInfo.depthTesting       = true;
    prefilterPipelineCreateInfo.depthWrite         = true;
    prefilterPipelineCreateInfo.renderPass         = prefilterRenderPass;
    auto prefilterPipeline = graphicsContext->createPipeline(&prefilterPipelineCreateInfo);
    auto environmentDescriptorSetPrefilter =
        graphicsContext->createDescriptorSet(prefilterPipeline, 0);
    graphicsContext->descriptorSetAddImage(environmentDescriptorSetPrefilter, 0, environmentMap);

    graphicsContext->beginRecording(equiToCubeCommandBuffer);

    struct PrefilterPushConstants {
        glm::mat4 view;
        float roughness;
    };

    unsigned int maxMipLevels = 5;
    for (unsigned int mipLevel = 0; mipLevel < maxMipLevels; mipLevel++) {
        for (int cubeFace = 0; cubeFace < 6; cubeFace++) {
            glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
            glm::mat4 viewMatrix(1.0f);
            switch (cubeFace) {
            case 0: {
                viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f));
                break;
            }
            case 1: {
                viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f));
                break;
            }
            case 2: {
                viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                                         glm::vec3(0.0f, 0.0f, 1.0f));
                break;
            }
            case 3: {
                viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                                         glm::vec3(0.0f, 0.0f, -1.0f));
                break;
            }
            case 4: {
                viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f));
                break;
            }
            case 5: {
                viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                         glm::vec3(0.0f, -1.0f, 0.0f));
                break;
            }
            }
            viewMatrix = captureProjection * viewMatrix;
            PrefilterPushConstants pushConstants;
            pushConstants.view      = viewMatrix;
            pushConstants.roughness = (float)mipLevel / (float)(maxMipLevels - 1);

            graphicsContext->beginRenderPass(equiToCubeCommandBuffer, prefilterRenderPass, 128,
                                             128);
            graphicsContext->bindPipeline(equiToCubeCommandBuffer, prefilterPipeline);
            graphicsContext->bindDescriptorSet(equiToCubeCommandBuffer, 0,
                                               environmentDescriptorSetPrefilter);
            graphicsContext->pushConstants(equiToCubeCommandBuffer, prefilterPipeline, 0,
                                           sizeof(PrefilterPushConstants), &pushConstants);
            graphicsContext->bindVertexBuffer(equiToCubeCommandBuffer, cubeVertexBuffer);
            graphicsContext->draw(equiToCubeCommandBuffer, (uint32_t)cubeVertices.size(), 1, 0, 0);

            graphicsContext->endRenderPass(equiToCubeCommandBuffer);

            graphicsContext->transitionRenderPassImages(
                equiToCubeCommandBuffer, prefilterRenderPass, ImageLayout::ATTACHMENT,
                ImageLayout::TRANSFER_SRC);

            unsigned int mipWidth  = (unsigned int)(128 * std::pow(0.5, mipLevel));
            unsigned int mipHeight = (unsigned int)(128 * std::pow(0.5, mipLevel));

            graphicsContext->blitRenderPassImageToCubemap(
                equiToCubeCommandBuffer, prefilterRenderPass, 0, prefilterMap, cubeFace, mipLevel,
                128, 128, mipWidth, mipHeight);
        }
    }
    graphicsContext->endRecording(equiToCubeCommandBuffer);
    graphicsContext->immediateSubmit(equiToCubeCommandBuffer);
}

void PBRRenderer::processBRDF(std::shared_ptr<RenderPass> brdfRenderPass) {
    PipelineCreateInfo brdfPipelineCreateInfo = {};
    brdfPipelineCreateInfo.vertexShaderPath   = "assets/shaders/brdf.vert";
    brdfPipelineCreateInfo.fragmentShaderPath = "assets/shaders/brdf.frag";
    brdfPipelineCreateInfo.viewportWidth      = 512;
    brdfPipelineCreateInfo.viewportHeight     = 512;
    brdfPipelineCreateInfo.culling            = false;
    brdfPipelineCreateInfo.depthTesting       = true;
    brdfPipelineCreateInfo.depthWrite         = true;
    brdfPipelineCreateInfo.renderPass         = brdfRenderPass;
    auto brdfPipeline = graphicsContext->createPipeline(&brdfPipelineCreateInfo);

    auto brdfCommandBuffer = graphicsContext->createCommandBuffer();

    graphicsContext->beginRecording(brdfCommandBuffer);
    graphicsContext->beginRenderPass(brdfCommandBuffer, brdfRenderPass, 512, 512);
    graphicsContext->bindPipeline(brdfCommandBuffer, brdfPipeline);
    graphicsContext->draw(brdfCommandBuffer, 3, 1, 0, 0);
    graphicsContext->endRenderPass(brdfCommandBuffer);

    graphicsContext->transitionRenderPassImages(brdfCommandBuffer, brdfRenderPass,
                                                ImageLayout::ATTACHMENT, ImageLayout::SHADER_READ);

    graphicsContext->endRecording(brdfCommandBuffer);

    graphicsContext->immediateSubmit(brdfCommandBuffer);
}
