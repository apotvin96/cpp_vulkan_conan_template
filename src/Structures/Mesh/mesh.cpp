#include "mesh.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

#include "../../Logger.hpp"

Mesh Mesh::loadFromObj(const char* filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, nullptr);
    if (!warn.empty()) {
        Logger::main_logger->warn("Object Load Warning: {0}", warn);
    }

    if (!err.empty()) {
        Logger::main_logger->error("Object Load Error: {0}", err);
    }

    std::vector<MeshVertex> vertices;

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

                MeshVertex new_vert;
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

    return Mesh(vertices);
}

Mesh Mesh::loadFromGltf(const char* filename) {
    tinygltf::Model model;
    std::string warning;
    std::string error;

    tinygltf::TinyGLTF loader;
    bool result = loader.LoadBinaryFromFile(&model, &error, &warning, filename);

    std::vector<MeshVertex> uniqueVerts;
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
        MeshVertex vertex;

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

    std::vector<MeshVertex> vertices;

    for (int i = 0; i < indices.size(); i++) {
        vertices.push_back(uniqueVerts[indices[i]]);
    }

    return Mesh(vertices);
}
