#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <3dmath.h>
#include <assimp/scene.h>


struct Mesh
{
  const uint32_t vertexArrayBufferObject;
  const int numIndices;

  std::vector<mat4x4> invBindPoses;
  std::map<std::string, int> mapOfNameInd;

  Mesh(uint32_t vertexArrayBufferObject, int numIndices) :
    vertexArrayBufferObject(vertexArrayBufferObject),
    numIndices(numIndices)
    {}
};

using MeshPtr = std::shared_ptr<Mesh>;

MeshPtr create_mesh(const aiMesh *mesh);
MeshPtr load_mesh(const char *path, int idx);
MeshPtr make_plane_mesh();
MeshPtr make_mesh(const std::vector<uint32_t> &indices, const std::vector<vec3> &vertices, const std::vector<vec3> &normals);

void render(const MeshPtr &mesh);
void render(const MeshPtr &mesh, int count);