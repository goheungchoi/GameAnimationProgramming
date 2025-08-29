/* model specific settings */
#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

struct InstanceSettings {
  glm::vec3 inst_WorldPosition = glm::vec3(0.0f);
  glm::vec3 inst_WorldRotation = glm::vec3(0.0f);
  float inst_Scale = 1.0f;
  bool inst_SwapYZAxis = false;

  unsigned int inst_AnimClipNr = 0;
  float inst_AnimPlayTimePos = 0.0f;
  float inst_AnimSpeedFactor = 1.0f;
};
