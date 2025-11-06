/* Simple camera object */
#pragma once

#include <glm/glm.hpp>

#include "VkRenderData.h"

class Camera {
 public:
  void addViewAzimuth(double dAzimuth);
  void addViewElevation(double dElevation);

	void addMoveForward(float dForward);
  void addMoveRight(float dRight);
  void addMoveUp(float dUp);
  void addMoveSpeed(float dSpeed);

	glm::vec3 getRotation() const;
	glm::vec3 getTranslation() const;
	float getMoveSpeed() const;

	void lookAt(glm::vec3 pos);
  void moveTo(glm::vec3 pos);

  void updateCamera(const float deltaTime);
  glm::mat4 getViewMatrix();

 private:
  float mViewAzimuth{330.0f};
  float mViewElevation{-20.0f};

  float mMoveForward{0.f};
  float mMoveRight{0.f};
	float mMoveUp{0.f};
	float mMoveSpeed{1.f};

  glm::vec3 mWorldPosition = glm::vec3(2.0f, 5.0f, 7.0f);

  glm::vec3 mViewDirection = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 mRightDirection = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 mUpDirection = glm::vec3(0.0f, 0.0f, 0.0f);

  /* world up is positive Y */
  static constexpr glm::vec3 kWorldUpVector = glm::vec3(0.0f, 1.0f, 0.0f);
	static constexpr glm::vec3 kWorldFrontVector = glm::vec3(0.0f, 0.0f, 1.0f);
	static constexpr glm::vec3 kWorldRightVector = glm::vec3(1.0f, 0.0f, 0.0f);
};
