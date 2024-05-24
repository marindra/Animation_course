#pragma once
#include <vector>
#include <3dmath.h>
#include <memory>
#include <assimp/scene.h>
#include <map>
#include <string>

class Skeleton
{
public:
  std::vector<glm::mat4x4> localTm;
  std::vector<glm::mat4x4> globalTm;
  std::vector<int> parentInd;
  std::map<std::string, int> mapOfNameInd;
  std::vector<std::string> names;

  int totalNodeCount = 0;
};

using SkeletonPtr = std::shared_ptr<const Skeleton>;

SkeletonPtr makeSkeleton(const aiNode* curNode);

class RuntimeSkeleton
{
public:
  SkeletonPtr baseSkeleton;
  std::vector<glm::mat4> localTm;
  std::vector<glm::mat4> globalTm;
  RuntimeSkeleton(SkeletonPtr ref) : baseSkeleton(ref), localTm(ref->localTm), globalTm(ref->totalNodeCount) {}

  void updateLocalTransforms();
};