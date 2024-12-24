#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "tiny_gltf.h"

#include "glfw/glfw3.h"

#include "renderer/GraphicsContext.hpp"
#include "renderer/Logger.hpp"

struct CameraData {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 viewProjection;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct VertexNormalMapped {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 uv;
};

std::vector<Vertex> loadFromObj(const char* filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, nullptr);
    if (!warn.empty()) {
        // Logger::main_logger->warn("Object Load Warning: {0}", warn);
    }
    if (!err.empty()) {
        Logger::main_logger->error("Object Load Error: {0}", err);
    }

    std::vector<Vertex> vertices;

    for (size_t s = 0; s < shapes.size(); s++) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            int fv = 3;

            for (size_t v = 0; v < fv; v++) {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                Vertex new_vert;
                new_vert.position.x = vx;
                new_vert.position.y = vy;
                new_vert.position.z = vz;

                new_vert.normal.x = nx;
                new_vert.normal.y = ny;
                new_vert.normal.z = nz;

                new_vert.uv.x = attrib.texcoords[2 * idx.texcoord_index + 0];
                new_vert.uv.y = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];

                vertices.push_back(new_vert);
            }
            index_offset += fv;
        }
    }

    return vertices;
}

std::vector<VertexNormalMapped> loadFromGltf(const char* filename) {
    tinygltf::Model model;
    std::string warning;
    std::string error;

    tinygltf::TinyGLTF loader;
    bool result = loader.LoadBinaryFromFile(&model, &error, &warning, filename);

    std::vector<VertexNormalMapped> uniqueVerts;
    std::vector<unsigned int> indices;

    tinygltf::Mesh mesh           = model.meshes[0];
    tinygltf::Primitive primitive = mesh.primitives[0];

    const tinygltf::Accessor& posAccessor     = model.accessors[primitive.attributes["POSITION"]];
    const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
    const tinygltf::Buffer& posBuffer         = model.buffers[posBufferView.buffer];
    const float* positions                    = reinterpret_cast<const float*>(
        &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);

    const tinygltf::Accessor& normAccessor     = model.accessors[primitive.attributes["NORMAL"]];
    const tinygltf::BufferView& normBufferView = model.bufferViews[normAccessor.bufferView];
    const tinygltf::Buffer& norm_buffer        = model.buffers[normBufferView.buffer];
    const float* normals                       = reinterpret_cast<const float*>(
        &norm_buffer.data[normBufferView.byteOffset + normAccessor.byteOffset]);

    const tinygltf::Accessor& tangentAccessor = model.accessors[primitive.attributes["TANGENT"]];
    const tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
    const tinygltf::Buffer& tangent_buffer        = model.buffers[tangentBufferView.buffer];
    const float* tangents                         = reinterpret_cast<const float*>(
        &tangent_buffer.data[tangentBufferView.byteOffset + tangentAccessor.byteOffset]);

    const tinygltf::Accessor& uvAccessor     = model.accessors[primitive.attributes["TEXCOORD_0"]];
    const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
    const tinygltf::Buffer& uvBuffer         = model.buffers[uvBufferView.buffer];
    const float* uvs                         = reinterpret_cast<const float*>(
        &uvBuffer.data[uvBufferView.byteOffset + uvAccessor.byteOffset]);

    const tinygltf::Accessor& indAccessor     = model.accessors[primitive.indices];
    const tinygltf::BufferView& indBufferView = model.bufferViews[indAccessor.bufferView];
    const tinygltf::Buffer& indBuffer         = model.buffers[indBufferView.buffer];

    for (int i = 0; i < posAccessor.count; i++) {
        VertexNormalMapped vertex;

        vertex.position.x = positions[i * 3 + 0];
        vertex.position.y = positions[i * 3 + 1];
        vertex.position.z = positions[i * 3 + 2];

        vertex.normal.x = normals[i * 3 + 0];
        vertex.normal.y = normals[i * 3 + 1];
        vertex.normal.z = normals[i * 3 + 2];

        vertex.tangent.x = tangents[i * 4 + 0];
        vertex.tangent.y = tangents[i * 4 + 1];
        vertex.tangent.z = tangents[i * 4 + 2];

        vertex.uv.x = uvs[i * 2 + 0];
        vertex.uv.y = uvs[i * 2 + 1];

        uniqueVerts.push_back(vertex);
    }

    if (indAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        const unsigned short* gltfIndices = reinterpret_cast<const unsigned short*>(
            &indBuffer.data[indBufferView.byteOffset + indAccessor.byteOffset]);
        for (size_t i = 0; i < indAccessor.count; i++) {
            indices.push_back(gltfIndices[i]);
        }
    } else if (indAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
        const unsigned int* gltfIndices = reinterpret_cast<const unsigned int*>(
            &indBuffer.data[indBufferView.byteOffset + indAccessor.byteOffset]);
        for (size_t i = 0; i < indAccessor.count; i++) {
            indices.push_back(gltfIndices[i]);
        }
    }

    std::vector<VertexNormalMapped> vertices;

    for (int i = 0; i < indices.size(); i++) {
        vertices.push_back(uniqueVerts[indices[i]]);
    }

    return vertices;
}

int main() {
    Logger::init();

    auto window = Window::create("PBR Demo", 1920, 1080);

    auto graphicsContext = GraphicsContext::create(window);

    auto renderFence      = graphicsContext->createFrameBasedFence(true);
    auto renderSemaphore  = graphicsContext->createFrameBasedSemaphore();
    auto presentSemaphore = graphicsContext->createFrameBasedSemaphore();

    auto mainCommandBuffer = graphicsContext->createFrameBasedCommandBuffer();

    // Create the environment and irradiance map
    auto environmentMap = graphicsContext->createCubemap(Format::RGBA16_FLOAT, 512, 512);
    auto irradianceMap  = graphicsContext->createCubemap(Format::RGBA16_FLOAT, 32, 32);
    auto prefilterMap   = graphicsContext->createCubemap(Format::RGBA16_FLOAT, 128, 128, true);

    {
        int width, height, numComp;
        // float* hdrData = stbi_loadf("assets/textures/wilderness.hdr", &width, &height, &numComp,
        // 4);
        float* hdrData =
            stbi_loadf("assets/textures/night_stars.hdr", &width, &height, &numComp, 4);
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

        std::vector<Vertex> cubeVertices = loadFromObj("assets/models/cube.obj");
        auto cubeVertexBuffer            = graphicsContext->createVertexBuffer(
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

            graphicsContext->beginRenderPass(equiToCubeCommandBuffer, equiToCubeRenderPass, 512,
                                             512);
            graphicsContext->bindPipeline(equiToCubeCommandBuffer, equiToCubePipeline);
            graphicsContext->bindVertexBuffer(equiToCubeCommandBuffer, cubeVertexBuffer);
            graphicsContext->pushConstants(equiToCubeCommandBuffer, equiToCubePipeline, 0,
                                           sizeof(glm::mat4), &viewMatrix);
            graphicsContext->bindDescriptorSet(equiToCubeCommandBuffer, 0, equiTextureSet);
            graphicsContext->draw(equiToCubeCommandBuffer, (uint32_t)cubeVertices.size(), 1, 0, 0);
            graphicsContext->endRenderPass(equiToCubeCommandBuffer);

            graphicsContext->transitionRenderPassImages(
                equiToCubeCommandBuffer, equiToCubeRenderPass, ImageLayout::ATTACHMENT,
                ImageLayout::TRANSFER_SRC);

            graphicsContext->copyRenderPassImageToCubemap(
                equiToCubeCommandBuffer, equiToCubeRenderPass, 0, environmentMap, i, 0, 512, 512);
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

        auto environmentMapDescriptorSet =
            graphicsContext->createDescriptorSet(convolutionPipeline, 0);
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

            graphicsContext->beginRenderPass(equiToCubeCommandBuffer, convolutionRenderPass, 32,
                                             32);
            graphicsContext->bindPipeline(equiToCubeCommandBuffer, convolutionPipeline);
            graphicsContext->bindDescriptorSet(equiToCubeCommandBuffer, 0,
                                               environmentMapDescriptorSet);
            graphicsContext->pushConstants(equiToCubeCommandBuffer, convolutionPipeline, 0,
                                           sizeof(glm::mat4), &viewMatrix);
            graphicsContext->bindVertexBuffer(equiToCubeCommandBuffer, cubeVertexBuffer);

            graphicsContext->draw(equiToCubeCommandBuffer, (uint32_t)cubeVertices.size(), 1, 0, 0);

            graphicsContext->endRenderPass(equiToCubeCommandBuffer);

            graphicsContext->transitionRenderPassImages(
                equiToCubeCommandBuffer, convolutionRenderPass, ImageLayout::ATTACHMENT,
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
        graphicsContext->descriptorSetAddImage(environmentDescriptorSetPrefilter, 0,
                                               environmentMap);

        graphicsContext->beginRecording(equiToCubeCommandBuffer);

        struct PrefilterPushConstants {
            glm::mat4 view;
            float roughness;
        };

        unsigned int maxMipLevels = 5;
        for (unsigned int mipLevel = 0; mipLevel < maxMipLevels; mipLevel++) {
            for (int cubeFace = 0; cubeFace < 6; cubeFace++) {
                glm::mat4 captureProjection =
                    glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
                glm::mat4 viewMatrix(1.0f);
                switch (cubeFace) {
                case 0: {
                    viewMatrix =
                        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                                    glm::vec3(0.0f, -1.0f, 0.0f));
                    break;
                }
                case 1: {
                    viewMatrix =
                        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                                    glm::vec3(0.0f, -1.0f, 0.0f));
                    break;
                }
                case 2: {
                    viewMatrix =
                        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                                    glm::vec3(0.0f, 0.0f, 1.0f));
                    break;
                }
                case 3: {
                    viewMatrix =
                        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                                    glm::vec3(0.0f, 0.0f, -1.0f));
                    break;
                }
                case 4: {
                    viewMatrix =
                        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                                    glm::vec3(0.0f, -1.0f, 0.0f));
                    break;
                }
                case 5: {
                    viewMatrix =
                        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
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
                graphicsContext->draw(equiToCubeCommandBuffer, (uint32_t)cubeVertices.size(), 1, 0,
                                      0);

                graphicsContext->endRenderPass(equiToCubeCommandBuffer);

                graphicsContext->transitionRenderPassImages(
                    equiToCubeCommandBuffer, prefilterRenderPass, ImageLayout::ATTACHMENT,
                    ImageLayout::TRANSFER_SRC);

                unsigned int mipWidth  = (unsigned int)(128 * std::pow(0.5, mipLevel));
                unsigned int mipHeight = (unsigned int)(128 * std::pow(0.5, mipLevel));

                graphicsContext->blitRenderPassImageToCubemap(
                    equiToCubeCommandBuffer, prefilterRenderPass, 0, prefilterMap, cubeFace,
                    mipLevel, 128, 128, mipWidth, mipHeight);
            }
        }
        graphicsContext->endRecording(equiToCubeCommandBuffer);
        graphicsContext->immediateSubmit(equiToCubeCommandBuffer);
    }

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

    {
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

        graphicsContext->transitionRenderPassImages(
            brdfCommandBuffer, brdfRenderPass, ImageLayout::ATTACHMENT, ImageLayout::SHADER_READ);

        graphicsContext->endRecording(brdfCommandBuffer);

        graphicsContext->immediateSubmit(brdfCommandBuffer);
    }

    // Create the main forward render pass
    std::vector<RenderPassAttachmentDescription> forwardPassAttachments;
    {
        RenderPassAttachmentDescription mainColorAttachment = {};
        mainColorAttachment.loadOp                          = LoadOp::CLEAR;
        mainColorAttachment.storeOp                         = StoreOp::STORE;
        mainColorAttachment.initialLayout                   = ImageLayout::UNDEFINED;
        mainColorAttachment.finalLayout                     = ImageLayout::ATTACHMENT;
        mainColorAttachment.format                          = Format::RGBA32_FLOAT;
        mainColorAttachment.width                           = window->getWidth();
        mainColorAttachment.height                          = window->getHeight();
        forwardPassAttachments.push_back(mainColorAttachment);
    }
    RenderPassAttachmentDescription forwardDepthAttachment = {};
    forwardDepthAttachment.loadOp                          = LoadOp::CLEAR;
    forwardDepthAttachment.storeOp                         = StoreOp::STORE;
    forwardDepthAttachment.initialLayout                   = ImageLayout::UNDEFINED;
    forwardDepthAttachment.finalLayout                     = ImageLayout::ATTACHMENT;

    // Create the main PBR pipeline for rendering
    PipelineCreateInfo pbrPipelineCreateInfo = {};
    pbrPipelineCreateInfo.vertexShaderPath   = "assets/shaders/pbr.vert";
    pbrPipelineCreateInfo.fragmentShaderPath = "assets/shaders/pbr.frag";
    pbrPipelineCreateInfo.viewportWidth      = window->getWidth();
    pbrPipelineCreateInfo.viewportHeight     = window->getHeight();
    pbrPipelineCreateInfo.culling            = false;
    pbrPipelineCreateInfo.depthTesting       = true;
    pbrPipelineCreateInfo.depthWrite         = true;
    auto pbrPipeline = graphicsContext->createPipeline(&pbrPipelineCreateInfo);

    auto cameraDescriptorSet = graphicsContext->createDescriptorSet(pbrPipeline, 0);
    graphicsContext->descriptorSetAddBuffer(cameraDescriptorSet, 0, DescriptorType::UNIFORM_BUFFER,
                                            sizeof(CameraData));
    graphicsContext->descriptorSetAddImage(cameraDescriptorSet, 1, irradianceMap);
    graphicsContext->descriptorSetAddImage(cameraDescriptorSet, 2, prefilterMap);
    graphicsContext->descriptorSetAddRenderPassAttachment(cameraDescriptorSet, 3, brdfRenderPass,
                                                          0);

    auto objectsDescriptorSet = graphicsContext->createDescriptorSet(pbrPipeline, 1);
    graphicsContext->descriptorSetAddBuffer(objectsDescriptorSet, 0, DescriptorType::STORAGE_BUFFER,
                                            sizeof(glm::mat4) * 10000);

    auto colorDescriptorSet = graphicsContext->createDescriptorSet(pbrPipeline, 2);
    int width, height, numComponents;
    unsigned char* grassData =
        stbi_load("assets/textures/metal.jpg", &width, &height, &numComponents, 4);
    auto grassTexture =
        graphicsContext->createTexture(width, height, 4, ColorSpace::SRGB, grassData, true);
    graphicsContext->descriptorSetAddImage(colorDescriptorSet, 0, grassTexture);
    unsigned char* materialData =
        stbi_load("assets/textures/metal_scratch_mat.png", &width, &height, &numComponents, 4);
    auto materialTexture =
        graphicsContext->createTexture(width, height, 4, ColorSpace::LINEAR, materialData, true);
    graphicsContext->descriptorSetAddImage(colorDescriptorSet, 1, materialTexture);
    unsigned char* normalData =
        stbi_load("assets/textures/metal_scratch_normal.jpg", &width, &height, &numComponents, 4);
    auto normalTexture =
        graphicsContext->createTexture(width, height, 4, ColorSpace::LINEAR, normalData, true);
    graphicsContext->descriptorSetAddImage(colorDescriptorSet, 2, normalTexture);

    // std::vector<VertexNormalMapped> vertices = loadFromGltf("assets/models/monkey.glb");
    std::vector<VertexNormalMapped> vertices = loadFromGltf("assets/models/sphere.glb");
    auto vertexBuffer                        = graphicsContext->createVertexBuffer(
        vertices.data(), uint32_t(vertices.size() * sizeof(VertexNormalMapped)));

    std::vector<Vertex> cubemapVertices = loadFromObj("assets/models/cube.obj");
    auto cubemapVertexBuffer            = graphicsContext->createVertexBuffer(
        cubemapVertices.data(), uint32_t(cubemapVertices.size() * sizeof(Vertex)));
    PipelineCreateInfo cubemapPipelineCreateInfo = {};
    cubemapPipelineCreateInfo.vertexShaderPath   = "assets/shaders/cubemap.vert";
    cubemapPipelineCreateInfo.fragmentShaderPath = "assets/shaders/cubemap.frag";
    cubemapPipelineCreateInfo.viewportWidth      = window->getWidth();
    cubemapPipelineCreateInfo.viewportHeight     = window->getHeight();
    cubemapPipelineCreateInfo.culling            = false;
    cubemapPipelineCreateInfo.depthTesting       = true;
    cubemapPipelineCreateInfo.depthWrite         = true;
    auto cubemapPipeline         = graphicsContext->createPipeline(&cubemapPipelineCreateInfo);
    auto cubeCameraDescriptorSet = graphicsContext->createDescriptorSet(cubemapPipeline, 0);
    graphicsContext->descriptorSetAddBuffer(cubeCameraDescriptorSet, 0,
                                            DescriptorType::UNIFORM_BUFFER, sizeof(CameraData));
    auto envMapDescriptorSet = graphicsContext->createDescriptorSet(cubemapPipeline, 1);
    // graphicsContext->descriptorSetAddImage(envMapDescriptorSet, 0, irradianceMap);
    graphicsContext->descriptorSetAddImage(envMapDescriptorSet, 0, environmentMap);

    glm::vec3 playerPos = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 playerRot = glm::vec3(0.0f, 0.0f, 0.0f);

    while (!window->shouldClose() && !window->keyDown(GLFW_KEY_ESCAPE)) {
        double startTime = glfwGetTime();
        Window::poll();

        if (window->keyDown(GLFW_KEY_W)) {
            playerPos.z -= 0.1f;
        }
        if (window->keyDown(GLFW_KEY_S)) {
            playerPos.z += 0.1f;
        }

        if (window->keyDown(GLFW_KEY_A)) {
            playerPos.x -= 0.1f;
        }
        if (window->keyDown(GLFW_KEY_D)) {
            playerPos.x += 0.1f;
        }

        if (window->keyDown(GLFW_KEY_LEFT)) {
            playerRot.y += 0.01f;
        }
        if (window->keyDown(GLFW_KEY_RIGHT)) {
            playerRot.y -= 0.01f;
        }

        if (graphicsContext->isSwapchainResized()) {
            // Create the main PBR pipeline for rendering
            pbrPipelineCreateInfo.viewportWidth  = window->getWidth();
            pbrPipelineCreateInfo.viewportHeight = window->getHeight();
            pbrPipeline = graphicsContext->createPipeline(&pbrPipelineCreateInfo);

            cameraDescriptorSet = graphicsContext->createDescriptorSet(pbrPipeline, 0);
            graphicsContext->descriptorSetAddBuffer(
                cameraDescriptorSet, 0, DescriptorType::UNIFORM_BUFFER, sizeof(CameraData));
            graphicsContext->descriptorSetAddImage(cameraDescriptorSet, 1, irradianceMap);
            graphicsContext->descriptorSetAddImage(cameraDescriptorSet, 2, prefilterMap);
            graphicsContext->descriptorSetAddRenderPassAttachment(cameraDescriptorSet, 3,
                                                                  brdfRenderPass, 0);

            objectsDescriptorSet = graphicsContext->createDescriptorSet(pbrPipeline, 1);
            graphicsContext->descriptorSetAddBuffer(
                objectsDescriptorSet, 0, DescriptorType::STORAGE_BUFFER, sizeof(glm::mat4) * 10000);

            colorDescriptorSet = graphicsContext->createDescriptorSet(pbrPipeline, 2);
            graphicsContext->descriptorSetAddImage(colorDescriptorSet, 0, grassTexture);
            graphicsContext->descriptorSetAddImage(colorDescriptorSet, 1, materialTexture);
            graphicsContext->descriptorSetAddImage(colorDescriptorSet, 2, normalTexture);

            cubemapPipelineCreateInfo.viewportWidth  = window->getWidth();
            cubemapPipelineCreateInfo.viewportHeight = window->getHeight();
            cubemapPipeline         = graphicsContext->createPipeline(&cubemapPipelineCreateInfo);
            cubeCameraDescriptorSet = graphicsContext->createDescriptorSet(cubemapPipeline, 0);
            graphicsContext->descriptorSetAddBuffer(
                cubeCameraDescriptorSet, 0, DescriptorType::UNIFORM_BUFFER, sizeof(CameraData));
            envMapDescriptorSet = graphicsContext->createDescriptorSet(cubemapPipeline, 1);
            graphicsContext->descriptorSetAddImage(envMapDescriptorSet, 0, irradianceMap);
        }

        graphicsContext->waitOnFence(renderFence);
        uint32_t swapchainImageIndex = graphicsContext->newFrame(presentSemaphore);

        graphicsContext->beginRecording(mainCommandBuffer);
        graphicsContext->beginSwapchainRenderPass(mainCommandBuffer, swapchainImageIndex,
                                                  glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

        graphicsContext->bindPipeline(mainCommandBuffer, pbrPipeline);
        glm::vec3 camPos = playerPos;
        glm::mat4 view   = glm::translate(
            glm::rotate(glm::mat4(1.0f), playerRot.y, glm::vec3(0.0, 1.0, 0.0)), camPos);
        glm::mat4 viewInverse = glm::inverse(view);
        glm::mat4 projection  = glm::perspective(glm::radians(45.f), 1600.f / 900.f, 0.1f, 200.0f);
        projection[1][1] *= -1;

        CameraData camData;
        camData.projection     = projection;
        camData.view           = viewInverse;
        camData.viewProjection = projection * viewInverse;
        void* memoryLocation   = graphicsContext->mapDescriptorBuffer(cameraDescriptorSet, 0);
        memcpy(memoryLocation, &camData, sizeof(CameraData));
        graphicsContext->unmapDescriptorBuffer(cameraDescriptorSet, 0);
        void* objectMemoryLocation  = graphicsContext->mapDescriptorBuffer(objectsDescriptorSet, 0);
        glm::mat4* objectMemoryMats = (glm::mat4*)objectMemoryLocation;
        objectMemoryMats[0] =
            glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0, 1, 0));
        graphicsContext->unmapDescriptorBuffer(objectsDescriptorSet, 0);
        graphicsContext->bindDescriptorSet(mainCommandBuffer, 0, cameraDescriptorSet);
        graphicsContext->bindDescriptorSet(mainCommandBuffer, 1, objectsDescriptorSet);
        graphicsContext->bindDescriptorSet(mainCommandBuffer, 2, colorDescriptorSet);

        glm::vec4 outCamPos = view[3];
        graphicsContext->pushConstants(mainCommandBuffer, pbrPipeline, 0, sizeof(glm::vec4),
                                       &outCamPos);

        graphicsContext->bindVertexBuffer(mainCommandBuffer, vertexBuffer);
        graphicsContext->draw(mainCommandBuffer, uint32_t(vertices.size()), 1, 0, 0);

        // Draw skybox
        graphicsContext->bindPipeline(mainCommandBuffer, cubemapPipeline);
        void* cubeCamMemoryLocation =
            graphicsContext->mapDescriptorBuffer(cubeCameraDescriptorSet, 0);
        memcpy(cubeCamMemoryLocation, &camData, sizeof(CameraData));
        graphicsContext->unmapDescriptorBuffer(cubeCameraDescriptorSet, 0);
        graphicsContext->bindDescriptorSet(mainCommandBuffer, 0, cubeCameraDescriptorSet);
        graphicsContext->bindDescriptorSet(mainCommandBuffer, 1, envMapDescriptorSet);
        graphicsContext->bindVertexBuffer(mainCommandBuffer, cubemapVertexBuffer);
        graphicsContext->draw(mainCommandBuffer, (uint32_t)cubemapVertices.size(), 1, 0, 0);

        graphicsContext->endRenderPass(mainCommandBuffer);

        graphicsContext->endRecording(mainCommandBuffer);

        graphicsContext->submit(mainCommandBuffer, presentSemaphore, renderSemaphore, renderFence);

        graphicsContext->present(swapchainImageIndex, renderSemaphore);
        Logger::main_logger->info("FPS: {0}", 1.0f / (glfwGetTime() - startTime));
    }

    graphicsContext->waitOnFence(renderFence, -1);

    return 0;
}