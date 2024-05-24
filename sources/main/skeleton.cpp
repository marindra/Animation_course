#include "skeleton.h"

static void iterate_over_tree(int parent_idx,const aiNode *node, Skeleton &skeleton)
{
  const aiMatrix4x4 &transform = node->mTransformation;
  glm::mat4 localTm = glm::mat4(
      transform.a1, transform.b1, transform.c1, transform.d1,
      transform.a2, transform.b2, transform.c2, transform.d2,
      transform.a3, transform.b3, transform.c3, transform.d3,
      transform.a4, transform.b4, transform.c4, transform.d4);

  int nodeInd = skeleton.totalNodeCount;
  ++skeleton.totalNodeCount;

  skeleton.mapOfNameInd.emplace(node->mName.C_Str(), nodeInd);
  skeleton.parentInd.emplace_back(parent_idx);
  skeleton.localTm.emplace_back(localTm);
  skeleton.names.emplace_back(node->mName.C_Str());

  for (size_t i = 0; i < node->mNumChildren; i++)
  {
    iterate_over_tree(nodeInd, node->mChildren[i], skeleton);
  }
}

SkeletonPtr makeSkeleton(const aiNode* curNode)
{
  Skeleton skeleton;
  iterate_over_tree(-1, curNode, skeleton);
  skeleton.globalTm = std::vector<glm::mat4>(skeleton.totalNodeCount + 1, glm::mat4(1.f));
  //skeleton.UpdateTransform();
  return std::make_shared<Skeleton>(std::move(skeleton));
}

void Skeleton::UpdateTransform()
{
  for (int i = 0; i < this->totalNodeCount; ++i)
  {
    int parent = this->parentInd[i];
    globalTm[i] = (parent >= 0 ? globalTm[parent] : glm::mat4(1.f)) * localTm[i];
  }
}