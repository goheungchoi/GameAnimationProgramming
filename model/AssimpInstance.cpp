#include "AssimpInstance.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Logger.h"

AssimpInstance::AssimpInstance(std::shared_ptr<AssimpModel> model,
                               glm::vec3 position, glm::vec3 rotation,
                               float modelScale)
    : mAssimpModel(model) {
  if (!model) {
    Logger::log(1, "%s error: invalid model given\n", __FUNCTION__);
    return;
  }
  mInstanceSettings.worldPosition = position;
  mInstanceSettings.worldRotation = rotation;
  mInstanceSettings.scale = modelScale;

  /* we need one 4x4 matrix for every bone */
  /*mBoneMatrices.resize(mAssimpModel->getBoneList().size());
  std::fill(mBoneMatrices.begin(), mBoneMatrices.end(), glm::mat4(1.0f));*/

  // avoid resizing during fill
  mNodeTransformData.resize(mAssimpModel->getBoneList().size());

	// save model root matrix
  mModelRootMatrix = mAssimpModel->getRootTranformationMatrix();

  updateModelRootMatrix();
}

void AssimpInstance::updateModelRootMatrix() {
  if (!mAssimpModel) {
    Logger::log(1, "%s error: invalid model\n", __FUNCTION__);
    return;
  }

  mLocalScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(mInstanceSettings.scale));

  if (mInstanceSettings.swapYZAxis) {
    glm::mat4 flipMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mLocalSwapAxisMatrix = glm::rotate(flipMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  } else {
    mLocalSwapAxisMatrix = glm::mat4(1.0f);
  }

  mLocalRotationMatrix = glm::mat4_cast(glm::quat(glm::radians(mInstanceSettings.worldRotation)));

  mLocalTranslationMatrix = glm::translate(glm::mat4(1.0f), mInstanceSettings.worldPosition);

  mLocalTransformMatrix = mLocalTranslationMatrix * mLocalRotationMatrix * mLocalSwapAxisMatrix * mLocalScaleMatrix;

	/* Update instance root transform matrix */
	mInstanceRootMatrix = mLocalTransformMatrix * mModelRootMatrix;
}

void AssimpInstance::updateAnimation(float deltaTime) {
  mInstanceSettings.animPlayTimePos += deltaTime * mAssimpModel->getAnimClips().at(mInstanceSettings.animClipNr)->getClipTicksPerSecond() * mInstanceSettings.animSpeedFactor;
  mInstanceSettings.animPlayTimePos = std::fmod(mInstanceSettings.animPlayTimePos, mAssimpModel->getAnimClips().at(mInstanceSettings.animClipNr)->getClipDuration());

  std::vector<std::shared_ptr<AssimpAnimChannel>> animChannels = mAssimpModel->getAnimClips().at(mInstanceSettings.animClipNr)->getChannels();

  /* animate clip via channels */
  for (const auto& channel : animChannels) {
    NodeTransformData nodeTransform;
    nodeTransform.translation =
        channel->getTranslation(mInstanceSettings.animPlayTimePos);
    nodeTransform.rotation =
        channel->getRotation(mInstanceSettings.animPlayTimePos);
    nodeTransform.scale =
        channel->getScaling(mInstanceSettings.animPlayTimePos);

    int boneId = channel->getBoneId();
    if (boneId >= 0) {
      mNodeTransformData.at(boneId) = nodeTransform;
    }
  }

  ///* set root node transform matrix, enabling instance movement */
  //mAssimpModel->getRootNode()->setRootTransformMatrix(mLocalTransformMatrix * mAssimpModel->getRootTranformationMatrix());

  ///* flat node map contains nodes in parent->child order, starting with root node, update matrices down the skeleton tree */
  //mBoneMatrices.clear();
  //for (auto& node : mAssimpModel->getNodeList()) {
  //  std::string nodeName = node->getNodeName();

  //  node->updateTRSMatrix();
  //  if (mAssimpModel->getBoneOffsetMatrices().count(nodeName) > 0) {
  //    mBoneMatrices.emplace_back(mAssimpModel->getNodeMap().at(nodeName)->getTRSMatrix() * mAssimpModel->getBoneOffsetMatrices().at(nodeName));
  //  }
  //}

	/* set root node transform matrix, enabling instance movement */
  updateModelRootMatrix();
}

std::shared_ptr<AssimpModel> AssimpInstance::getModel() {
  return mAssimpModel;
}

glm::vec3 AssimpInstance::getWorldPosition() {
  return mInstanceSettings.worldPosition;
}

glm::mat4 AssimpInstance::getWorldTransformMatrix() {
  return mLocalTransformMatrix;
}

void AssimpInstance::setTranslation(glm::vec3 position) {
  mInstanceSettings.worldPosition = position;
  updateModelRootMatrix();
}

void AssimpInstance::setRotation(glm::vec3 rotation) {
  mInstanceSettings.worldRotation = rotation;
  updateModelRootMatrix();
}

void AssimpInstance::setScale(float scale) {
  mInstanceSettings.scale = scale;
  updateModelRootMatrix();
}

void AssimpInstance::setSwapYZAxis(bool value) {
  mInstanceSettings.swapYZAxis = value;
  updateModelRootMatrix();
}

glm::vec3 AssimpInstance::getRotation() {
  return mInstanceSettings.worldRotation;
}

glm::vec3 AssimpInstance::getTranslation() {
  return mInstanceSettings.worldPosition;
}

float AssimpInstance::getScale() {
  return mInstanceSettings.scale;
}

bool AssimpInstance::getSwapYZAxis() {
  return mInstanceSettings.swapYZAxis;
}

void AssimpInstance::setInstanceSettings(InstanceSettings settings) {
  mInstanceSettings = settings;
  updateModelRootMatrix();
}

InstanceSettings AssimpInstance::getInstanceSettings() {
  return mInstanceSettings;
}

//std::vector<glm::mat4> AssimpInstance::getBoneMatrices() {
//  return mBoneMatrices;
//}

const std::vector<NodeTransformData>& AssimpInstance::getNodeTransformData()
    const {
  return mNodeTransformData;
}
