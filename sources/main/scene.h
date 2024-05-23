#pragma once
#include <render/mesh.h>
#include "skeleton.h"



struct SceneAsset
{
  MeshPtr mesh;
  SkeletonPtr skeleton;
};

using SceneAssetPtr = std::shared_ptr<SceneAsset>;

SceneAssetPtr makeScene(const char *path, int idx);