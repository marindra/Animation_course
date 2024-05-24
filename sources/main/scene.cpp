#include "scene.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <log.h>

SceneAssetPtr makeScene(const char *path, int idx)
{
  Assimp::Importer importer;
  importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
  importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 1.f);

  importer.ReadFile(path, aiPostProcessSteps::aiProcess_Triangulate | aiPostProcessSteps::aiProcess_LimitBoneWeights |
    aiPostProcessSteps::aiProcess_GenNormals | aiProcess_GlobalScale | aiProcess_FlipWindingOrder | aiProcess_PopulateArmatureData);

  const aiScene* scene = importer.GetScene();
  if (!scene)
  {
    debug_error("no asset in %s", path);
    return nullptr;
  }

  MeshPtr curMesh = create_mesh(scene->mMeshes[idx]);
  RuntimeSkeleton curSkeleton = RuntimeSkeleton(makeSkeleton(scene->mRootNode));
  importer.FreeScene();
  SceneAsset curScene{std::move(curMesh), std::move(curSkeleton)};
  return std::make_shared<SceneAsset>(std::move(curScene));
}