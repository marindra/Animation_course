#include "scene.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <log.h>

SkeletonPtr create_skeleton(const aiNode &ai_node);
AnimationPtr create_animation(const aiAnimation &ai_animation, const SkeletonPtr &skeleton, AnimationInfo* animInfo = nullptr);

SceneAssetPtr makeScene(const char *path, int load_flags, AnimationInfo* animInfo)
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

  SceneAsset curScene;
  if (load_flags & SceneAsset::LoadScene::Meshes)
  {
    curScene.meshes.reserve(scene->mNumMeshes);
    for (size_t i = 0; i < scene->mNumMeshes; i++)
      curScene.meshes.emplace_back(create_mesh(scene->mMeshes[i]));
  }
  if (load_flags & SceneAsset::LoadScene::Skeleton)
  {
  curScene.skeleton = create_skeleton(*scene->mRootNode);
  }
  if (load_flags & SceneAsset::LoadScene::Animation)
  {
    curScene.animations.reserve(scene->mNumAnimations);
    for (size_t i = 0; i < scene->mNumAnimations; i++)
      if (AnimationPtr animation = create_animation(*scene->mAnimations[i], curScene.skeleton, animInfo))
        curScene.animations.emplace_back(std::move(animation));
  }
  //MeshPtr curMesh = create_mesh(scene->mMeshes[idx]);
  //SkeletonPtr curSkeleton = RuntimeSkeleton(makeSkeleton(scene->mRootNode));
  importer.FreeScene();
  return std::make_shared<SceneAsset>(std::move(curScene));
}