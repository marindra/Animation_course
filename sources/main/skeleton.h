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

  int totalNodeCount = 0;

  void UpdateTransform();
};

using SkeletonPtr = std::shared_ptr<Skeleton>;

SkeletonPtr makeSkeleton(const aiNode* curNode);