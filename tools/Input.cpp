#include "Input.h"

void InputManager::KeyBinding::remove() {
  if (_conn && _conn->isConnected()) {
    _conn->getHost()->keyBindingSlots.removeKeyBindingSlot(id);
  }
}

void InputManager::process() {
  pollEvents();

  processKeyBindings();
}

void InputManager::pushKeyEvent(const KeyEvent& e) {
  std::lock_guard<std::mutex> lock(getCurrEventQueueMutex());

  CircularBuffer& buf = getCurrEventQueue();
  buf.write<KeyEvent>(&e);
}

InputManager::KeyBinding InputManager::bindKey(Delegate<void()> d, int key,
                                 KeyActionType action, int priority,
                                 bool consume) {
  uint32_t id = keyBindingSlots.addKeyBindingSlot(std::move(d), key, action, priority,
                                         consume);
  return KeyBinding{id, _conn};
}	

void InputManager::pollEvents() {
  std::lock_guard<std::mutex> lock(getCurrEventQueueMutex());

  CircularBuffer& buf = getCurrEventQueue();

  InputEventHeader header;
  while (buf.peek(&header)) {
    switch (header.type) {
      case InputEventType_Keyboard:
        readKeyEvent(buf);
        break;
      case InputEventType_MouseButton:
      case InputEventType_MousePosition:
      case InputEventType_GamePadKey:
      case InputEventType_GamePadJoystick:
      default:
        break;
    }
  }

  ++indexCount;
}

void InputManager::readKeyEvent(CircularBuffer& buf) {
  KeyEvent e;
  buf.read(&e);
  switch (e.action) {
    case GLFW_PRESS:
      currKeys.set(e.key);
      break;
    case GLFW_RELEASE:
      currKeys.reset(e.key);
      break;
  }
}

