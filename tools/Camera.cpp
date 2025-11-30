#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

void Camera::addViewAzimuth(double dAzimuth) {
  mViewAzimuth += dAzimuth;
  /* keep between 0 and 360 degree */
  if (mViewAzimuth < 0.0f) {
    mViewAzimuth += 360.0f;
  }
  if (mViewAzimuth >= 360.0f) {
    mViewAzimuth -= 360.0f;
  }
}

void Camera::addViewElevation(double dElevation) {
  mViewElevation += dElevation;
  /* keep between -89 and +89 degree */
  mViewElevation = std::clamp(mViewElevation, -89.0f, 89.0f);
}

void Camera::addMoveForward(float dForward) {
	mMoveForward += dForward;
}

void Camera::addMoveRight(float dRight) {
	mMoveRight += dRight;
}

void Camera::addMoveUp(float dUp) {
	mMoveUp += dUp;
}

void Camera::addMoveSpeed(float dSpeed) {
	mMoveSpeed += dSpeed;
  /* keep between 1 and 100 */
	mMoveSpeed = std::clamp(mMoveSpeed, 1.f, 1000.f);
}

glm::vec3 Camera::getRotation() const {
  return {0.f, mViewElevation, mViewAzimuth};
}

glm::vec3 Camera::getTranslation() const {
	return mWorldPosition; }

float Camera::getViewAzimuth() const { return mViewAzimuth; }

float Camera::getViewElevation() const { return mViewElevation; }

float Camera::getMoveSpeed() const {
	return mMoveSpeed; }

void Camera::lookAt(glm::vec3 pos) {
  glm::vec3 dir = glm::normalize(pos - mWorldPosition);
  mViewAzimuth = glm::degrees(atan2f(dir.x, -dir.z));
  mViewElevation = glm::degrees(asinf(dir.y));
}

void Camera::moveTo(glm::vec3 pos) {
  mWorldPosition = pos;
}

void Camera::updateCamera(const float deltaTime) {
  if (deltaTime == 0.0f) {
    return;
  }

  float azimRad = glm::radians(mViewAzimuth);
  float elevRad = glm::radians(mViewElevation);

  float sinAzim = std::sin(azimRad);
  float cosAzim = std::cos(azimRad);
  float sinElev = std::sin(elevRad);
  float cosElev = std::cos(elevRad);

  /* update view direction */
  mViewDirection = glm::normalize(glm::vec3(
    sinAzim * cosElev, sinElev, -cosAzim * cosElev));

  /* calculate right and up direction */
  mRightDirection = glm::normalize(glm::cross(mViewDirection, kWorldUpVector));
  mUpDirection = glm::normalize(glm::cross(mRightDirection, mViewDirection));

  /* update camera position depending on desired movement */
	glm::vec3 dForward = deltaTime * mMoveSpeed * mMoveForward * mViewDirection;
  glm::vec3 dRight = deltaTime * mMoveSpeed * mMoveRight * mRightDirection;
	glm::vec3 dUp = deltaTime * mMoveSpeed * mMoveUp * mUpDirection;
  mWorldPosition += dForward;
	mWorldPosition += dRight;
	mWorldPosition += dUp;

	/* reset delta movement */
	mMoveForward = 0.f;
	mMoveRight = 0.f;
	mMoveUp = 0.f;
}

glm::mat4 Camera::getViewMatrix() {
  glm::vec3 eye = mWorldPosition;
	glm::vec3 at = mWorldPosition + mViewDirection;
	glm::vec3 up = mUpDirection;

  return glm::lookAt(eye, at, up);
}
