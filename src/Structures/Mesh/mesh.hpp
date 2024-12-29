#pragma once

#include <vector>
#include "mesh_vertex.hpp"

class Mesh {
public:
    static Mesh loadFromObj(const char* filename);
    static Mesh loadFromGltf(const char* filename);

    Mesh() : vertices() {}
    Mesh(std::vector<MeshVertex> vertices) : vertices(vertices) {}

    std::vector<MeshVertex> vertices;
};