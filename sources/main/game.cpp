
#include <render/direction_light.h>
#include <render/material.h>
#include <render/mesh.h>
#include "camera.h"
#include <application.h>
#include <render/debug_arrow.h>
#include "scene.h"
#include <imgui/imgui.h>
#include "ImGuizmo.h"

struct UserCamera
{
  glm::mat4 transform;
  mat4x4 projection;
  ArcballCamera arcballCamera;
};

struct Character
{
  glm::mat4 transform;
  // MeshPtr mesh;
  MaterialPtr material;
  // SkeletonPtr skeleton;
  SceneAssetPtr fullObject;
};

struct Scene
{
  DirectionLight light;

  UserCamera userCamera;

  std::vector<Character> characters;

};

static std::unique_ptr<Scene> scene;

vec3 get_random_color(uint x)
{
  x += 1u;
  vec3 col = vec3(1.61803398875);
  col = fract(col) * vec3(x,x,x);
  col = fract(col) * vec3(1,x,x);
  col = fract(col) * vec3(1,1,x);
  //col = vec3(phi*i, phi*i*i, phi*i*i*i); // has precision issues
  return fract(col);
}

void game_init()
{
  scene = std::make_unique<Scene>();
  scene->light.lightDirection = glm::normalize(glm::vec3(-1, -1, 0));
  scene->light.lightColor = glm::vec3(1.f);
  scene->light.ambient = glm::vec3(0.2f);

  scene->userCamera.projection = glm::perspective(90.f * DegToRad, get_aspect_ratio(), 0.01f, 500.f);

  ArcballCamera &cam = scene->userCamera.arcballCamera;
  cam.curZoom = cam.targetZoom = 0.5f;
  cam.maxdistance = 5.f;
  cam.distance = cam.curZoom * cam.maxdistance;
  cam.lerpStrength = 10.f;
  cam.mouseSensitivity = 0.5f;
  cam.wheelSensitivity = 0.05f;
  cam.targetPosition = glm::vec3(0.f, 1.f, 0.f);
  cam.targetRotation = cam.curRotation = glm::vec2(DegToRad * -90.f, DegToRad * -30.f);
  cam.rotationEnable = false;

  scene->userCamera.transform = calculate_transform(scene->userCamera.arcballCamera);

  input.onMouseButtonEvent += [](const SDL_MouseButtonEvent &e) { arccam_mouse_click_handler(e, scene->userCamera.arcballCamera); };
  input.onMouseMotionEvent += [](const SDL_MouseMotionEvent &e) { arccam_mouse_move_handler(e, scene->userCamera.arcballCamera); };
  input.onMouseWheelEvent += [](const SDL_MouseWheelEvent &e) { arccam_mouse_wheel_handler(e, scene->userCamera.arcballCamera); };


  auto material = make_material("character", "sources/shaders/character_vs.glsl", "sources/shaders/character_ps.glsl");
  std::fflush(stdout);
  material->set_property("mainTex", create_texture2d("resources/MotusMan_v55/MCG_diff.jpg"));

  scene->characters.emplace_back(Character{
    glm::identity<glm::mat4>(),
    //load_mesh("resources/MotusMan_v55/MotusMan_v55.fbx", 0),
    std::move(material),
    makeScene("resources/MotusMan_v55/MotusMan_v55.fbx", 0)
  });

  create_arrow_render();

  std::fflush(stdout);
}


void game_update()
{
  arcball_camera_update(
    scene->userCamera.arcballCamera,
    scene->userCamera.transform,
    get_delta_time());
}

void render_imguizmo(ImGuizmo::OPERATION &mCurrentGizmoOperation, ImGuizmo::MODE &mCurrentGizmoMode)
{
  if (ImGui::Begin("gizmo window"))
  {
    if (ImGui::IsKeyPressed('Z'))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed('E'))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed('R')) // r Key
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
      mCurrentGizmoOperation = ImGuizmo::SCALE;

    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    {
      if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        mCurrentGizmoMode = ImGuizmo::LOCAL;
      ImGui::SameLine();
      if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        mCurrentGizmoMode = ImGuizmo::WORLD;
    }
  }
  ImGui::End();
}

void imgui_render()
{
  ImGuizmo::BeginFrame();
  static size_t idx = 3;
  for (Character &character : scene->characters)
  {
    //character.skeleton.updateLocalTransforms();
    character.fullObject->skeleton.updateLocalTransforms();
    //const RuntimeSkeleton &skeleton = character.skeleton;
    if (ImGui::Begin("Skeleton view"))
    {
      for (const auto& elem : character.fullObject->skeleton.baseSkeleton->mapOfNameInd)
      {
        ImGui::Text("%d) %s", elem.second, elem.first.c_str());
        ImGui::SameLine();
        ImGui::PushID(elem.second);
        if (ImGui::Button("edit"))
        {
          idx = elem.second;
        }
        ImGui::PopID();
      }
      //for (size_t i = 0; i < nodeCount; i++)
      //{
      //  ImGui::Text("%d) %s", int(i), skeleton.ref->names[i].c_str());
      //}
    }
    ImGui::End();

    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
    render_imguizmo(mCurrentGizmoOperation, mCurrentGizmoMode);

    const glm::mat4 &projection = scene->userCamera.projection;
    const glm::mat4 &transform = scene->userCamera.transform;
    mat4 cameraView = inverse(transform);
 
    ImGuiIO &io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    glm::mat4 globNodeTm = character.fullObject->skeleton.globalTm[idx]; //character.skeleton.globalTm[idx];

    ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode,
                         glm::value_ptr(globNodeTm));

    int parent = character.fullObject->skeleton.baseSkeleton->parentInd[idx]; //skeleton.ref->parent[idx];
    //character.skeleton.localTm[idx] = glm::inverse(parent >= 0 ? character.skeleton.globalTm[parent] : glm::mat4(1.f)) * globNodeTm;
    character.fullObject->skeleton.localTm[idx] = glm::inverse(parent >= 0 ? character.fullObject->skeleton.globalTm[parent] :\
      glm::mat4(1.f)) * globNodeTm;

    break;
  }
}

void render_character(const Character &character, const mat4 &cameraProjView, vec3 cameraPosition, const DirectionLight &light)
{
  const Material &material = *character.material;
  const Shader &shader = material.get_shader();

  size_t meshBoneCount = character.fullObject->mesh->invBindPoses.size();
  std::vector<mat4> bones(meshBoneCount);

  shader.use();
  material.bind_uniforms_to_shader();
  //shader.set_mat4x4("Bones", {});
  shader.set_mat4x4("Transform", character.transform);
  shader.set_mat4x4("ViewProjection", cameraProjView);
  shader.set_vec3("CameraPosition", cameraPosition);
  shader.set_vec3("LightDirection", glm::normalize(light.lightDirection));
  shader.set_vec3("AmbientLight", light.ambient);
  shader.set_vec3("SunLight", light.lightColor);

  render(character.fullObject->mesh); //character.mesh);

  //for (size_t i = 0; i < character.fullObject->mesh->bones.size(); ++i)
  //{
    //const auto &bone = character.fullObject->mesh->bones[i];
    //draw_arrow(character.fullObject->mesh->bones[bone.parentId].bindPose * vec4(0, 0, 0, 1), bone.bindPose * vec4(0, 0, 0, 1), vec3(0, 0.5f, 0.3f), 0.01f);

    //if (character.fullObject->mesh->bones[bone.parentId].bindPose != character.fullObject->skeleton->globalTm[\
    //character.fullObject->skeleton->mapOfNameInd[character.fullObject->mesh->bones[bone.parentId].name]])
    //{
    //  printf("Differences in position: %s\n", character.fullObject->mesh->bones[bone.parentId].name.c_str());
    //}
  //}
  //printf("\n\n");

  for (int i = 0; i < character.fullObject->skeleton.baseSkeleton->totalNodeCount; ++i)
  {
    const auto& it = character.fullObject->mesh->mapOfNameInd.find(character.fullObject->skeleton.baseSkeleton->names[i]);
    if (it != character.fullObject->mesh->mapOfNameInd.end())
    {
      int boneIdx = it->second;
      bones[boneIdx] = character.fullObject->skeleton.globalTm[i] * character.fullObject->mesh->invBindPoses[boneIdx];
    }
  }
  shader.set_mat4x4("Bones", bones);

  render(character.fullObject->mesh);

  for (int i = 0; i < character.fullObject->skeleton.baseSkeleton->totalNodeCount; ++i)
  {
    int parentIdx = character.fullObject->skeleton.baseSkeleton->parentInd[i];
    assert(parentIdx < i);
    if (parentIdx >= 0)
    {
      draw_arrow(character.fullObject->skeleton.globalTm[parentIdx][3],\
      character.fullObject->skeleton.globalTm[i][3], /*vec3(0, 0.3f, 0.5f)*/get_random_color(i), 0.01f);
    }
  }

}

void game_render()
{
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  const float grayColor = 0.3f;
  glClearColor(grayColor, grayColor, grayColor, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  const mat4 &projection = scene->userCamera.projection;
  const glm::mat4 &transform = scene->userCamera.transform;
  mat4 projView = projection * inverse(transform);

  for (const Character &character : scene->characters)
    render_character(character, projView, glm::vec3(transform[3]), scene->light);
  
  render_arrows(projView, glm::vec3(transform[3]), scene->light);
}