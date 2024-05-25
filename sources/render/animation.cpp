#include <assimp/scene.h>
#include <main/scene.h>
#include <log.h>

#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/offline/raw_animation.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton_utils.h"

void buildSkeleton(ozz::animation::offline::RawSkeleton::Joint &root, const aiNode &aiRoot)
{
  root.name = aiRoot.mName.C_Str();

  aiVector3D scaling;
  aiQuaternion rotation;
  aiVector3D position;
  aiRoot.mTransformation.Decompose(scaling, rotation, position);

  root.transform.translation = ozz::math::Float3(position.x, position.y, position.z);
  root.transform.rotation = ozz::math::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
  root.transform.scale = ozz::math::Float3(scaling.x, scaling.y, scaling.z);

  // Now adds children to the root.
  root.children.resize(aiRoot.mNumChildren);
  for (size_t i = 0; i < aiRoot.mNumChildren; ++i)
  {
    buildSkeleton(root.children[i], *aiRoot.mChildren[i]);
  }
}

SkeletonPtr create_skeleton(const aiNode &ai_root)
{
  ozz::animation::offline::RawSkeleton raw_skeleton;

  // Creates the root joint.
  raw_skeleton.roots.resize(1);
  buildSkeleton(raw_skeleton.roots[0], ai_root);
  ozz::animation::offline::RawSkeleton::Joint& root = raw_skeleton.roots[0];

  // Setups the 1st child name (left) and transfomation.
  /*ozz::animation::offline::RawSkeleton::Joint& left = root.children[0];
  left.name = "left";
  left.transform.translation = ozz::math::Float3(1.f, 0.f, 0.f);
  left.transform.rotation = ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f);
  left.transform.scale = ozz::math::Float3(1.f, 1.f, 1.f);

  // Setups the 2nd child name (right) and transfomation.
  ozz::animation::offline::RawSkeleton::Joint& right = root.children[1];
  right.name = "right";
  right.transform.translation = ozz::math::Float3(-1.f, 0.f, 0.f);
  right.transform.rotation = ozz::math::Quaternion(0.f, 0.f, 0.f, 1.f);
  right.transform.scale = ozz::math::Float3(1.f, 1.f, 1.f);

  //...and so on with the whole skeleton hierarchy...*/

  // Test for skeleton validity.
  // The main invalidity reason is the number of joints, which must be lower
  // than ozz::animation::Skeleton::kMaxJoints.
  if (!raw_skeleton.Validate()) {
    debug_log("Skeleton building failed.");
    return nullptr;
  }

  //////////////////////////////////////////////////////////////////////////////
  // This final section converts the RawSkeleton to a runtime Skeleton.
  //////////////////////////////////////////////////////////////////////////////

  // Creates a SkeletonBuilder instance.
  ozz::animation::offline::SkeletonBuilder builder;

  // Executes the builder on the previously prepared RawSkeleton, which returns
  // a new runtime skeleton instance.
  // This operation will fail and return an empty unique_ptr if the RawSkeleton
  // isn't valid.
  ozz::unique_ptr<ozz::animation::Skeleton> skeleton = builder(raw_skeleton);

  return std::shared_ptr<ozz::animation::Skeleton>(std::move(skeleton));
}

AnimationPtr create_animation(const aiAnimation &ai_animation, const SkeletonPtr &skeleton)
{
// Creates a RawAnimation.
  ozz::animation::offline::RawAnimation raw_animation;
  double secPerTick = 1.0 / ai_animation.mTicksPerSecond;
  raw_animation.duration = ai_animation.mDuration * secPerTick;
  raw_animation.name = ai_animation.mName.C_Str();

  // Creates 3 animation tracks.
  // There should be as much tracks as there are joints in the skeleton that
  // this animation targets.
  raw_animation.tracks.resize(skeleton->num_joints());

  // Fills each track with keyframes, in joint local-space.
  // Tracks should be ordered in the same order as joints in the
  // ozz::animation::Skeleton. Joint's names can be used to find joint's
  // index in the skeleton.

  for (int jointIdx = 0; jointIdx < skeleton->num_joints(); ++jointIdx)
  {
    int channelIdx = -1;
    for (size_t i = 0; i < ai_animation.mNumChannels; ++i)
    {
      if (strcmp(ai_animation.mChannels[i]->mNodeName.C_Str(), skeleton->joint_names()[jointIdx]) == 0)
      {
        channelIdx = i;
        break;
      }
    }

    ozz::animation::offline::RawAnimation::JointTrack &track = raw_animation.tracks[jointIdx];
    if (channelIdx > 0)
    {
      const aiNodeAnim &channel = *ai_animation.mChannels[channelIdx];
      //ozz::animation::offline::RawAnimation::JointTrack &track = raw_animation.tracks[jointIdx];

      track.translations.reserve(channel.mNumPositionKeys);
      for (size_t i = 0; i < channel.mNumPositionKeys; ++i)
      {
        track.translations.push_back(ozz::animation::offline::RawAnimation::TranslationKey{\
          float(channel.mPositionKeys[i].mTime * secPerTick), ozz::math::Float3(\
            channel.mPositionKeys[i].mValue.x,\
            channel.mPositionKeys[i].mValue.y,\
            channel.mPositionKeys[i].mValue.z)});
      }

      track.rotations.reserve(channel.mNumRotationKeys);
      for (size_t i = 0; i < channel.mNumRotationKeys; ++i)
      {
        track.rotations.push_back(ozz::animation::offline::RawAnimation::RotationKey{\
          float(channel.mRotationKeys[i].mTime * secPerTick), ozz::math::Quaternion(\
            channel.mRotationKeys[i].mValue.x,\
            channel.mRotationKeys[i].mValue.y,\
            channel.mRotationKeys[i].mValue.z,\
            channel.mRotationKeys[i].mValue.w)});
      }

      track.scales.reserve(channel.mNumScalingKeys);
      for (size_t i = 0; i < channel.mNumScalingKeys; ++i)
      {
        track.scales.push_back(ozz::animation::offline::RawAnimation::ScaleKey{\
          float(channel.mScalingKeys[i].mTime * secPerTick), ozz::math::Float3(\
            channel.mScalingKeys[i].mValue.x,\
            channel.mScalingKeys[i].mValue.y,\
            channel.mScalingKeys[i].mValue.z)});
      }
    }
    else
    {
      ozz::math::Transform jointTm = ozz::animation::GetJointLocalRestPose(*skeleton, jointIdx);

      track.translations =
          {ozz::animation::offline::RawAnimation::TranslationKey{0.f, jointTm.translation}};
      track.rotations =
          {ozz::animation::offline::RawAnimation::RotationKey{0.f, jointTm.rotation}};
      track.scales =
          {ozz::animation::offline::RawAnimation::ScaleKey{0.f, jointTm.scale}};
    }
  }

  // Test for animation validity. These are the errors that could invalidate
  // an animation:
  //  1. Animation duration is less than 0.
  //  2. Keyframes' are not sorted in a strict ascending order.
  //  3. Keyframes' are not within [0, duration] range.
  if (!raw_animation.Validate()) {
    debug_log("Animation building failed.");
    return nullptr;
  }

  //////////////////////////////////////////////////////////////////////////////
  // This final section converts the RawAnimation to a runtime Animation.
  //////////////////////////////////////////////////////////////////////////////

  // Creates a AnimationBuilder instance.
  ozz::animation::offline::AnimationBuilder builder;

  // Executes the builder on the previously prepared RawAnimation, which returns
  // a new runtime animation instance.
  // This operation will fail and return an empty unique_ptr if the RawAnimation
  // isn't valid.
  ozz::unique_ptr<ozz::animation::Animation> animation = builder(raw_animation);

  // ...use the animation as you want...
  //ozz::animation::offline::AnimationOptimizer optimizer

  return std::shared_ptr<ozz::animation::Animation>(std::move(animation));
}