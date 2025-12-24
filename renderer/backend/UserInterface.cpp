#include "UserInterface.h"

#include <ImGuiFileDialog.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/string_cast.hpp>
#include <limits>
#include <string>

#include "AssimpAnimClip.h"
#include "AssimpInstance.h"
#include "AssimpModel.h"
#include "AssimpSettingsContainer.h"
#include "Camera.h"
#include "CommandBuffer.h"
#include "ImGuizmo.h"
#include "InstanceSettings.h"
#include "Logger.h"

bool UserInterface::init(VkRenderData& renderData) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  std::vector<VkDescriptorPoolSize> imguiPoolSizes = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo imguiPoolInfo{};
  imguiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  imguiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  imguiPoolInfo.maxSets = 1000;
  imguiPoolInfo.poolSizeCount = static_cast<uint32_t>(imguiPoolSizes.size());
  imguiPoolInfo.pPoolSizes = imguiPoolSizes.data();

  VkResult result =
      vkCreateDescriptorPool(renderData.rdVkbDevice.device, &imguiPoolInfo,
                             nullptr, &renderData.rdImguiDescriptorPool);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not init ImGui descriptor pool \n",
                __FUNCTION__);
    return false;
  }

  if (!ImGui_ImplGlfw_InitForVulkan(renderData.rdWindow, true)) {
    Logger::log(1, "%s error: could not init ImGui GLFW for Vulkan \n",
                __FUNCTION__);
    return false;
  }

  ImGui_ImplVulkan_InitInfo imguiIinitInfo{};
  imguiIinitInfo.Instance = renderData.rdVkbInstance.instance;
  imguiIinitInfo.PhysicalDevice =
      renderData.rdVkbPhysicalDevice.physical_device;
  imguiIinitInfo.Device = renderData.rdVkbDevice.device;
  imguiIinitInfo.Queue = renderData.rdGraphicsQueue;
  imguiIinitInfo.DescriptorPool = renderData.rdImguiDescriptorPool;
  imguiIinitInfo.MinImageCount = 2;
  imguiIinitInfo.ImageCount =
      static_cast<uint32_t>(renderData.rdSwapchainImages.size());
  imguiIinitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  imguiIinitInfo.RenderPass = renderData.rdRenderpass;

  if (!ImGui_ImplVulkan_Init(&imguiIinitInfo)) {
    Logger::log(1, "%s error: could not init ImGui for Vulkan \n",
                __FUNCTION__);
    return false;
  }

  ImGui::StyleColorsDark();

  /* init plot vectors */
  mFPSValues.resize(mNumFPSValues);
  mFrameTimeValues.resize(mNumFrameTimeValues);
  mModelUploadValues.resize(mNumModelUploadValues);
  mUpdateAnimationValues.resize(mNumUpdateAnimationValues);
  mMatrixUploadValues.resize(mNumMatrixUploadValues);
  mUiGenValues.resize(mNumUiGenValues);
  mUiDrawValues.resize(mNumUiDrawValues);

  return true;
}

void UserInterface::hideMouse(bool hide) {
  /* v1.89.8 removed the check for disabled mouse cursor in GLFW
   * we need to ignore the mouse postion if the mouse lock is active */
  ImGuiIO& io = ImGui::GetIO();

  if (hide) {
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
  } else {
    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
  }
}

void UserInterface::createFrame(VkRenderData& renderData,
                                ModelAndInstanceData& modInstData,
                                Camera* cam) {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGuizmo::BeginFrame();

  ImGuiIO& io = ImGui::GetIO();
  ImGuiWindowFlags imguiWindowFlags = 0;

  ImGui::SetNextWindowBgAlpha(0.8f);

  /* dim background for modal dialogs */
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.75f);

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Edit")) {
      if (modInstData.miSettingsContainer->getUndoSize() == 0) {
        ImGui::BeginDisabled();
      }
      if (ImGui::MenuItem("Undo", "CTRL+Z")) {
        modInstData.miUndoCallbackFunction();
      }
      if (modInstData.miSettingsContainer->getUndoSize() == 0) {
        ImGui::EndDisabled();
      }

      if (modInstData.miSettingsContainer->getRedoSize() == 0) {
        ImGui::BeginDisabled();
      }
      if (ImGui::MenuItem("Redo", "CTRL+Y")) {
        modInstData.miRedoCallbackFunction();
      }
      if (modInstData.miSettingsContainer->getRedoSize() == 0) {
        ImGui::EndDisabled();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  /* avoid inf values (division by zero) */
  if (renderData.rdFrameTime > 0.0) {
    mNewFps = 1.0f / renderData.rdFrameTime * 1000.f;
  }
  /* make an averge value to avoid jumps */
  mFramesPerSecond =
      (mAveragingAlpha * mFramesPerSecond) + (1.0f - mAveragingAlpha) * mNewFps;

  /* clamp manual input on all sliders to min/max */
  ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

  /* avoid literal double compares */
  if (mUpdateTime < 0.000001) {
    mUpdateTime = ImGui::GetTime();
  }

  while (mUpdateTime < ImGui::GetTime()) {
    mFPSValues.at(mFpsOffset) = mFramesPerSecond;
    mFpsOffset = ++mFpsOffset % mNumFPSValues;

    mFrameTimeValues.at(mFrameTimeOffset) = renderData.rdFrameTime;
    mFrameTimeOffset = ++mFrameTimeOffset % mNumFrameTimeValues;

    mModelUploadValues.at(mModelUploadOffset) = renderData.rdUploadToVBOTime;
    mModelUploadOffset = ++mModelUploadOffset % mNumModelUploadValues;

    mUpdateAnimationValues.at(mUpdateAnimOffset) =
        renderData.rdUpdateAnimationTime;
    mUpdateAnimOffset = ++mUpdateAnimOffset % mNumUpdateAnimationValues;

    float totalMatrixUploadTime =
        renderData.rdUploadToSSBOTime + renderData.rdUploadToUBOTime;
    mMatrixUploadValues.at(mMatrixUploadOffset) = totalMatrixUploadTime;
    mMatrixUploadOffset = ++mMatrixUploadOffset % mNumMatrixUploadValues;

    mUiGenValues.at(mUiGenOffset) = renderData.rdUIGenerateTime;
    mUiGenOffset = ++mUiGenOffset % mNumUiGenValues;

    mUiDrawValues.at(mUiDrawOffset) = renderData.rdUIDrawTime;
    mUiDrawOffset = ++mUiDrawOffset % mNumUiDrawValues;

    mUpdateTime += 1.0 / 30.0;
  }

  if (!ImGui::Begin("Control", nullptr, imguiWindowFlags)) {
    /* window collapsed */
    ImGui::End();
    return;
  }

  ImGui::Text("FPS: %10.4f", mFramesPerSecond);

  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    float averageFPS = 0.0f;
    for (const auto value : mFPSValues) {
      averageFPS += value;
    }
    averageFPS /= static_cast<float>(mNumFPSValues);
    std::string fpsOverlay = "now:     " + std::to_string(mFramesPerSecond) +
                             "\n30s avg: " + std::to_string(averageFPS);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("FPS");
    ImGui::SameLine();
    ImGui::PlotLines("##FrameTimes", mFPSValues.data(), mFPSValues.size(),
                     mFpsOffset, fpsOverlay.c_str(), 0.0f,
                     std::numeric_limits<float>::max(), ImVec2(0, 80));
    ImGui::EndTooltip();
  }

  if (ImGui::CollapsingHeader("Info")) {
    ImGui::Text("Triangles:              %10i", renderData.rdTriangleCount);

    std::string unit = "B";
    float memoryUsage = renderData.rdMatricesSize;

    if (memoryUsage > 1024.0f * 1024.0f) {
      memoryUsage /= 1024.0f * 1024.0f;
      unit = "MB";
    } else if (memoryUsage > 1024.0f) {
      memoryUsage /= 1024.0f;
      unit = "KB";
    }

    ImGui::Text("Instance Matrix Size:  %8.2f %2s", memoryUsage, unit.c_str());

    std::string windowDims = std::to_string(renderData.rdWidth) + "x" +
                             std::to_string(renderData.rdHeight);
    ImGui::Text("Window Dimensions:      %10s", windowDims.c_str());

    std::string imgWindowPos =
        std::to_string(static_cast<int>(ImGui::GetWindowPos().x)) + "/" +
        std::to_string(static_cast<int>(ImGui::GetWindowPos().y));
    ImGui::Text("ImGui Window Position:  %10s", imgWindowPos.c_str());
  }

  if (ImGui::CollapsingHeader("Timers")) {
    ImGui::Text("Frame Time:             %10.4f ms", renderData.rdFrameTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageFrameTime = 0.0f;
      for (const auto value : mFrameTimeValues) {
        averageFrameTime += value;
      }
      averageFrameTime /= static_cast<float>(mNumUpdateAnimationValues);
      std::string frameTimeOverlay =
          "now:     " + std::to_string(renderData.rdFrameTime) +
          " ms\n30s avg: " + std::to_string(averageFrameTime) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Frame Time       ");
      ImGui::SameLine();
      ImGui::PlotLines("##FrameTime", mFrameTimeValues.data(),
                       mFrameTimeValues.size(), mFrameTimeOffset,
                       frameTimeOverlay.c_str(), 0.0f,
                       std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Model Upload Time:      %10.4f ms",
                renderData.rdUploadToVBOTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();

      float averageModelUpload = 0.0f;
      for (const auto value : mModelUploadValues) {
        averageModelUpload += value;
      }
      averageModelUpload /= static_cast<float>(mNumModelUploadValues);
      std::string modelUploadOverlay =
          "now:     " + std::to_string(renderData.rdUploadToVBOTime) +
          " ms\n30s avg: " + std::to_string(averageModelUpload) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("VBO Upload");
      ImGui::SameLine();
      ImGui::PlotLines("##ModelUploadTimes", mModelUploadValues.data(),
                       mModelUploadValues.size(), mModelUploadOffset,
                       modelUploadOverlay.c_str(), 0.0f,
                       std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("Update Animation Time: %10.4f ms",
                renderData.rdUpdateAnimationTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatGen = 0.0f;
      for (const auto value : mUpdateAnimationValues) {
        averageMatGen += value;
      }
      averageMatGen /= static_cast<float>(mNumUpdateAnimationValues);
      std::string updateAnimOverlay =
          "now:     " + std::to_string(renderData.rdUpdateAnimationTime) +
          " ms\n30s avg: " + std::to_string(averageMatGen) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Update Animation");
      ImGui::SameLine();
      ImGui::PlotLines("##UpdateAnimTimes", mUpdateAnimationValues.data(),
                       mUpdateAnimationValues.size(), mUpdateAnimOffset,
                       updateAnimOverlay.c_str(), 0.0f,
                       std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    float totalMatrixUploadTime =
        renderData.rdUploadToUBOTime + renderData.rdUploadToSSBOTime;

    ImGui::Text("Matrix Upload Time:     %10.4f ms", totalMatrixUploadTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageMatrixUpload = 0.0f;
      for (const auto value : mMatrixUploadValues) {
        averageMatrixUpload += value;
      }
      averageMatrixUpload /= static_cast<float>(mNumMatrixUploadValues);
      std::string matrixUploadOverlay =
          "now:     " + std::to_string(totalMatrixUploadTime) +
          " ms\n30s avg: " + std::to_string(averageMatrixUpload) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Matrix Upload");
      ImGui::SameLine();
      ImGui::PlotLines("##MatrixUploadTimes", mMatrixUploadValues.data(),
                       mMatrixUploadValues.size(), mMatrixUploadOffset,
                       matrixUploadOverlay.c_str(), 0.0f,
                       std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("UI Generation Time:     %10.4f ms",
                renderData.rdUIGenerateTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageUiGen = 0.0f;
      for (const auto value : mUiGenValues) {
        averageUiGen += value;
      }
      averageUiGen /= static_cast<float>(mNumUiGenValues);
      std::string uiGenOverlay =
          "now:     " + std::to_string(renderData.rdUIGenerateTime) +
          " ms\n30s avg: " + std::to_string(averageUiGen) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("UI Generation");
      ImGui::SameLine();
      ImGui::PlotLines("##UIGenTimes", mUiGenValues.data(), mUiGenValues.size(),
                       mUiGenOffset, uiGenOverlay.c_str(), 0.0f,
                       std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }

    ImGui::Text("UI Draw Time:           %10.4f ms", renderData.rdUIDrawTime);

    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      float averageUiDraw = 0.0f;
      for (const auto value : mUiDrawValues) {
        averageUiDraw += value;
      }
      averageUiDraw /= static_cast<float>(mNumUiDrawValues);
      std::string uiDrawOverlay =
          "now:     " + std::to_string(renderData.rdUIDrawTime) +
          " ms\n30s avg: " + std::to_string(averageUiDraw) + " ms";
      ImGui::AlignTextToFramePadding();
      ImGui::Text("UI Draw");
      ImGui::SameLine();
      ImGui::PlotLines("##UIDrawTimes", mUiDrawValues.data(),
                       mUiDrawValues.size(), mUiDrawOffset,
                       uiDrawOverlay.c_str(), 0.0f,
                       std::numeric_limits<float>::max(), ImVec2(0, 80));
      ImGui::EndTooltip();
    }
  }

  if (ImGui::CollapsingHeader("Camera")) {
    ImGui::Text("Camera Position: %s",
                glm::to_string(cam->getTranslation()).c_str());
    ImGui::Text("View Azimuth:    %6.1f", cam->getViewAzimuth());
    ImGui::Text("View Elevation:  %6.1f", cam->getViewElevation());
  }

  if (ImGui::CollapsingHeader("Models")) {
    /* state is changed during model deletion, so save it first */
    bool modelListEmtpy = modInstData.miModelList.size() == 1;
    std::string selectedModelName = "None";

    if (!modelListEmtpy) {
      selectedModelName =
          modInstData.miModelList.at(modInstData.miSelectedModel)
              ->getModelFileName()
              .c_str();
    }

    if (modelListEmtpy) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Models :");
    ImGui::SameLine();
    ImGui::PushItemWidth(200);
    if (ImGui::BeginCombo("##ModelCombo",
                          // avoid access the empty model vector
                          selectedModelName.c_str())) {
      for (int i = 0; i < modInstData.miModelList.size(); ++i) {
        const bool isSelected = (modInstData.miSelectedModel == i);
        if (ImGui::Selectable(
                modInstData.miModelList.at(i)->getModelFileName().c_str(),
                isSelected)) {
          modInstData.miSelectedModel = i;
          selectedModelName =
              modInstData.miModelList.at(modInstData.miSelectedModel)
                  ->getModelFileName()
                  .c_str();
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if (modelListEmtpy) {
      ImGui::EndDisabled();
    }

    if (ImGui::Button("Import Model")) {
      IGFD::FileDialogConfig config;
      config.path = ".";
      config.countSelectionMax = 1;
      config.flags = ImGuiFileDialogFlags_Modal;
      ImGui::SetNextWindowPos(
          ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
          ImGuiCond_Always, ImVec2(0.5f, 0.5f));
      ImGuiFileDialog::Instance()->OpenDialog(
          "ChooseModelFile", "Choose Model File",
          "Supported Model Files{.gltf,.glb,.obj,.fbx,.dae,.mdl,.md3,.pk3}",
          config);
    }

    if (ImGuiFileDialog::Instance()->Display("ChooseModelFile")) {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        std::string filePathName =
            ImGuiFileDialog::Instance()->GetFilePathName();

        /* try to construct a relative path */
        std::filesystem::path currentPath = std::filesystem::current_path();
        std::string relativePath =
            std::filesystem::relative(filePathName, currentPath)
                .generic_string();

        if (!relativePath.empty()) {
          filePathName = relativePath;
        }
        /* Windows does understand forward slashes, but std::filesystem
         * preferres backslashes... */
        std::replace(filePathName.begin(), filePathName.end(), '\\', '/');

        if (!modInstData.miModelAddCallbackFunction(filePathName)) {
          Logger::log(
              1, "%s error: unable to load model file '%s', unknown error \n",
              __FUNCTION__, filePathName.c_str());
        } else {
          /* select new model and new instance */
          modInstData.miSelectedModel = modInstData.miModelList.size() - 1;
          modInstData.miSelectedInstance =
              modInstData.miAssimpInstances.size() - 1;
        }
      }
      ImGuiFileDialog::Instance()->Close();
    }

    if (modelListEmtpy) {
      ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Model")) {
      ImGui::OpenPopup("Delete Model?");
    }

    if (ImGui::BeginPopupModal("Delete Model?", nullptr,
                               ImGuiChildFlags_AlwaysAutoResize)) {
      ImGui::Text("Delete Model '%s'?",
                  modInstData.miModelList.at(modInstData.miSelectedModel)
                      ->getModelFileName()
                      .c_str());

      /* cheating a bit to get buttons more to the center */
      ImGui::Indent();
      ImGui::Indent();
      if (ImGui::Button("OK") ||
          ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter))) {
        modInstData.miModelDeleteCallbackFunction(
            modInstData.miModelList.at(modInstData.miSelectedModel)
                ->getModelFileName()
                .c_str());

        /* decrement selected model index to point to model that is in list
         * before the deleted one */
        if (modInstData.miSelectedModel > 0) {
          modInstData.miSelectedModel -= 1;
        }

        /* reset model instance to first instnace - if we have instances */
        if (!modInstData.miAssimpInstances.empty()) {
          modInstData.miSelectedInstance = 0;
        }
        ImGui::CloseCurrentPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel") ||
          ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Create Instance")) {
      std::shared_ptr<AssimpModel> currentModel =
          modInstData.miModelList[modInstData.miSelectedModel];
      modInstData.miInstanceAddCallbackFunction(currentModel);
      /* select new instance */
      modInstData.miSelectedInstance = modInstData.miAssimpInstances.size() - 1;
    }

    if (ImGui::Button("Create Multiple Instances")) {
      std::shared_ptr<AssimpModel> currentModel =
          modInstData.miModelList[modInstData.miSelectedModel];
      modInstData.miInstanceAddManyCallbackFunction(currentModel,
                                                    mManyInstanceCreateNum);
      modInstData.miSelectedInstance = modInstData.miAssimpInstances.size() - 1;
    }
    ImGui::SameLine();
    ImGui::SliderInt("##MassInstanceCreation", &mManyInstanceCreateNum, 1, 100,
                     "%d", flags);

    if (modelListEmtpy) {
      ImGui::EndDisabled();
    }
  }

  if (ImGui::CollapsingHeader("Instances")) {
    bool modelListEmtpy = modInstData.miModelList.size() == 1;
    bool nullInstanceSelected = modInstData.miSelectedInstance == 0;
    size_t numberOfInstances = modInstData.miAssimpInstances.size() - 1;

    ImGui::Text("Number of Instances: %ld", numberOfInstances);

    if (modelListEmtpy) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Hightlight Instance:");
    ImGui::SameLine();
    ImGui::Checkbox("##HighlightInstance",
                    &renderData.rdHighlightSelectedInstance);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Selected Instance  :");
    ImGui::SameLine();
    ImGui::PushButtonRepeat(true);
    if (ImGui::ArrowButton("##Left", ImGuiDir_Left) &&
        modInstData.miSelectedInstance > 1) {
      modInstData.miSelectedInstance--;
    }

    if (modelListEmtpy || nullInstanceSelected) {
      ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(30);
    ImGui::DragInt("##SelInst", &modInstData.miSelectedInstance, 1, 1,
                   modInstData.miAssimpInstances.size() - 1, "%3d", flags);
    ImGui::PopItemWidth();

    if (modelListEmtpy || nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::ArrowButton("##Right", ImGuiDir_Right) &&
        modInstData.miSelectedInstance <
            (modInstData.miAssimpInstances.size() - 1)) {
      modInstData.miSelectedInstance++;
    }
    ImGui::PopButtonRepeat();

    if (nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    if (modelListEmtpy || nullInstanceSelected) {
      ImGui::BeginDisabled();
    }

    /* DragInt does not like clamp flag */
    modInstData.miSelectedInstance =
        std::clamp(modInstData.miSelectedInstance, 0,
                   static_cast<int>(modInstData.miAssimpInstances.size() - 1));

    InstanceSettings settings;
    if (numberOfInstances > 0) {
      settings =
          modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)
              ->getInstanceSettings();
      // check if currently selected instance is different
      // from the new current instance.
      if (mCurrentInstance !=
          modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)) {
        mCurrentInstance =
            modInstData.miAssimpInstances.at(modInstData.miSelectedInstance);
        mSavedInstanceSettings = settings;
      }
    }

    if (ImGui::Button("Center This Instance")) {
      modInstData.miInstanceCenterCallbackFunction(mCurrentInstance);
    }

    ImGui::SameLine();

    /* we MUST retain the last model */
    unsigned int numberOfInstancesPerModel = 0;
    if (modInstData.miAssimpInstances.size() > 1) {
      std::string currentModelName =
          mCurrentInstance->getModel()->getModelFileName();
      numberOfInstancesPerModel =
          modInstData.miAssimpInstancesPerModel[currentModelName].size();
    }

    if (numberOfInstancesPerModel < 2) {
      ImGui::BeginDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Delete Instance")) {
      modInstData.miInstanceDeleteCallbackFunction(mCurrentInstance);

      /* hard reset for now */
      if (modInstData.miSelectedInstance > 1) {
        modInstData.miSelectedInstance -= 1;
      }
      settings =
          modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)
              ->getInstanceSettings();
    }

    if (numberOfInstancesPerModel < 2) {
      ImGui::EndDisabled();
    }

    if (ImGui::Button("Clone Instance")) {
      modInstData.miInstanceCloneCallbackFunction(mCurrentInstance);

      /* reset to last position for now */
      modInstData.miSelectedInstance = modInstData.miAssimpInstances.size() - 1;

      /* read back settings for UI */
      settings =
          modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)
              ->getInstanceSettings();
    }

    // if (ImGui::Button("Create Multiple Clones")) {
    //   modInstData.miInstanceCloneManyCallbackFunction(
    //       mCurrentInstance, mManyInstanceCloneNum);

    //  /* reset to last position for now */
    //  modInstData.miSelectedInstance =
    //      modInstData.miAssimpInstances.size() - 1;

    //  /* read back settings for UI */
    //  settings = modInstData.miAssimpInstances
    //                  .at(modInstData.miSelectedInstance)
    //                  ->getInstanceSettings();
    //}
    ImGui::SameLine();
    ImGui::SliderInt("##MassInstanceCloning", &mManyInstanceCloneNum, 1, 100,
                     "%d", flags);

    if (modelListEmtpy || nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    /* get the new size, in case of a deletion */
    numberOfInstances = modInstData.miAssimpInstances.size() - 1;

    std::string baseModelName = "None";
    if (numberOfInstances > 0 && !nullInstanceSelected) {
      baseModelName =
          modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)
              ->getModel()
              ->getModelFileName();
    }
    ImGui::Text("Base Model: %s", baseModelName.c_str());

    if (numberOfInstances == 0 || nullInstanceSelected) {
      ImGui::BeginDisabled();
    }

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Swap Y and Z axes:     ");
    ImGui::SameLine();
    ImGui::Checkbox("##ModelAxisSwap", &settings.swapYZAxis);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Model Pos (X/Y/Z):     ");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelPos", glm::value_ptr(settings.worldPosition),
                        -25.0f, 25.0f, "%.3f", flags);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Model Rotation (X/Y/Z):");
    ImGui::SameLine();
    ImGui::SliderFloat3("##ModelRot", glm::value_ptr(settings.worldRotation),
                        -180.0f, 180.0f, "%.3f", flags);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Model Scale:           ");
    ImGui::SameLine();
    ImGui::SliderFloat("##ModelScale", &settings.scale, 0.001f, 10.0f, "%.4f",
                       flags);

    if (ImGui::Button("Reset Instance Values")) {
      InstanceSettings defaultSettings{};

      /* save and restore index positions */
      int instanceIndex = settings.instanceIndexPos;
      settings = defaultSettings;
      settings.instanceIndexPos = instanceIndex;
    }

    if (numberOfInstances == 0 || nullInstanceSelected) {
      ImGui::EndDisabled();
    }

    if (numberOfInstances > 0) {
      modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)
          ->setInstanceSettings(settings);
    }
  }

  if (ImGui::CollapsingHeader("Animations")) {
    size_t numberOfInstances = modInstData.miAssimpInstances.size() - 1;

    InstanceSettings settings;
    size_t numberOfClips = 0;
    if (numberOfInstances > 0) {
      settings =
          modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)
              ->getInstanceSettings();
      numberOfClips =
          modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)
              ->getModel()
              ->getAnimClips()
              .size();
    }

    if (numberOfInstances > 0 && numberOfClips > 0) {
      std::vector<std::shared_ptr<AssimpAnimClip>> animClips =
          modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)
              ->getModel()
              ->getAnimClips();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Animation Clip:");
      ImGui::SameLine();
      if (ImGui::BeginCombo(
              "##ClipCombo",
              animClips.at(settings.animClipNr)->getClipName().c_str())) {
        for (int i = 0; i < animClips.size(); ++i) {
          const bool isSelected = (settings.animClipNr == i);
          if (ImGui::Selectable(animClips.at(i)->getClipName().c_str(),
                                isSelected)) {
            settings.animClipNr = i;
          }

          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Replay Speed:  ");
      ImGui::SameLine();
      ImGui::SliderFloat("##ClipSpeed", &settings.animSpeedFactor, 0.0f, 2.0f,
                         "%.3f", flags);
    } else {
      /* TODO: better solution if no instances or no clips are found */
      ImGui::BeginDisabled();

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Animation Clip:");
      ImGui::SameLine();
      ImGui::BeginCombo("##ClipComboDisabled", "None");

      float playSpeed = 1.0f;
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Replay Speed:  ");
      ImGui::SameLine();
      ImGui::SliderFloat("##ClipSpeedDisabled", &playSpeed, 0.0f, 2.0f, "%.3f",
                         flags);

      ImGui::EndDisabled();
    }

    if (numberOfInstances > 0) {
      modInstData.miAssimpInstances.at(modInstData.miSelectedInstance)
          ->setInstanceSettings(settings);
    }
  }

  if (modInstData.miSelectedInstance > 0) {
    /* draw coordinate lines */
    static ImGuizmo::OPERATION gOp = ImGuizmo::TRANSLATE;
    static ImGuizmo::MODE gMode = ImGuizmo::LOCAL;
    static bool gUseSnap = false;
    static float gSnapMove[3] = {0.1f, 0.1f, 0.1f};  // step in world units
    static float gSnapRot = 5.0f;                    // degrees
    static float gSnapScale = 0.1f;

    // hotkeys when instance is selected.
    if (ImGui::IsKeyPressed(ImGuiKey_W)) gOp = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_E)) gOp = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R)) gOp = ImGuizmo::SCALE_Y;
    if (ImGui::IsKeyPressed(ImGuiKey_Q))
      gMode = (gMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

    auto pInst =
        modInstData.miAssimpInstances.at(modInstData.miSelectedInstance);
    InstanceSettings inst = pInst->getInstanceSettings();

    glm::vec3 trans = pInst->getTranslation();
    glm::vec3 rot = pInst->getRotation();
    float scale = pInst->getScale();
    glm::mat4 M = glm::mat4(1.f);
    M = glm::translate(M, trans);
    M = M * glm::mat4_cast(glm::quat(glm::radians(rot)));
    M = glm::scale(M, glm::vec3(scale));

    ImGui::InputFloat3("T", glm::value_ptr(trans));
    ImGui::InputFloat3("R", glm::value_ptr(rot));
    ImGui::InputFloat("S", &scale);

    if (ImGui::IsKeyPressed(ImGuiKey_LeftCtrl)) gUseSnap = true;
    ImGui::Checkbox("", &gUseSnap);
    ImGui::SameLine();

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    glm::mat4 view = cam->getViewMatrix();
    glm::mat4 proj = glm::perspective(
        glm::radians(static_cast<float>(renderData.rdFOV)),
        static_cast<float>(renderData.rdVkbSwapchain.extent.width) /
            static_cast<float>(renderData.rdVkbSwapchain.extent.height),
        0.1f, 500.0f);
    if (ImGuizmo::Manipulate(
            glm::value_ptr(view), glm::value_ptr(proj), gOp, gMode,
            glm::value_ptr(M), NULL, gUseSnap ? gSnapMove : NULL)) {
      glm::vec3 newScale;
      glm::quat newRot;
      glm::vec3 newTrans;
      glm::vec3 skew;
      glm::vec4 perspective;
      glm::decompose(M, newScale, newRot, newTrans, skew, perspective);

      if (gOp == ImGuizmo::OPERATION::TRANSLATE)
        pInst->setTranslation(newTrans);
      else if (gOp == ImGuizmo::OPERATION::ROTATE)
        pInst->setRotation(glm::degrees(glm::eulerAngles(newRot)));
      else if (gOp == ImGuizmo::OPERATION::SCALE_Y)
        pInst->setScale(newScale.y);
    }

		mShouldSaveInstanceSettings =
        !ImGuizmo::IsUsing() && (mPrevManipulationState ^ ImGuizmo::IsUsing());
    if (mShouldSaveInstanceSettings) {
      modInstData.miApplyCallbackFunction(mSavedInstanceSettings);
		}
    mPrevManipulationState = ImGuizmo::IsUsing();
  }

  ImGui::End();
}

void UserInterface::render(VkRenderData& renderData) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  renderData.rdCommandBuffer);
}

void UserInterface::cleanup(VkRenderData& renderData) {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();

  vkDestroyDescriptorPool(renderData.rdVkbDevice.device,
                          renderData.rdImguiDescriptorPool, nullptr);
  ImGui::DestroyContext();
}
