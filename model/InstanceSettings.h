/* model specific settings */
#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

struct InstanceSettings {
  glm::vec3 worldPosition = glm::vec3(0.0f);
  glm::vec3 worldRotation = glm::vec3(0.0f);
  float scale = 1.0f;
  bool swapYZAxis = false;

  unsigned int animClipNr = 0;
  float animPlayTimePos = 0.0f;
  float animSpeedFactor = 1.0f;
};
