#include "Input.h"

void InputManager::KeyBinding::remove() {
  if (_conn && _conn->isConnected()) {
    _conn->getHost()->keyBindingSlots.removeKeyBindingSlot(id);
  }
}

void InputManager::MouseMoveBinding::remove() {
  if (_conn && _conn->isConnected()) {
    _conn->getHost()->mouseMoveBindingSlots.removeKeyBindingSlot(id);
  }
}

void InputManager::process() {
  pollEvents();

  processKeyBindings();
  processMouseMoveBindings();
}

void InputManager::pushKeyEvent(const KeyEvent& e) {
  std::lock_guard<std::mutex> lock(queueMutexes[getWriteQueueIndex()]);

  CircularBuffer& buf = getCurrEventQueue();
  buf.write<KeyEvent>(&e);
}

void InputManager::pushMousePositionEvent(const MousePositionEvent& e) {
  std::lock_guard<std::mutex> lock(queueMutexes[getWriteQueueIndex()]);

  CircularBuffer& buf = getCurrEventQueue();
  buf.write<MousePositionEvent>(&e);
}

InputManager::KeyBinding InputManager::bindKey(Delegate<void()> d, int key,
                                               KeyActionType action,
                                               int priority, bool consume) {
  uint32_t id = keyBindingSlots.addBindingSlot(std::move(d), key, action,
                                               priority, consume);
  return KeyBinding{id, _conn};
}
InputManager::MouseMoveBinding InputManager::bindMouseMove(
    Delegate<void(float, float)> d, MouseMode mode, int priority,
    bool consume) {
  uint32_t id = mouseMoveBindingSlots.addBindingSlot(std::move(d), mode,
                                                         priority, consume);

  return MouseMoveBinding{id, _conn};
}

void InputManager::pollEvents() {
  const uint8_t prev = writeIndex.exchange(
      (uint8_t)((getWriteQueueIndex() + 1) & 1), std::memory_order_acq_rel);
  std::lock_guard<std::mutex> lock(queueMutexes[prev]);

  CircularBuffer& buf = eventQueues[prev];

  InputEventHeader header;
  while (buf.peek(&header)) {
    switch (header.type) {
      case InputEventType_Keyboard:
        readKeyEvent(buf);
        break;
      case InputEventType_MouseButton:
      case InputEventType_MousePosition:
        readMousePositionEvent(buf);
        break;
      case InputEventType_GamePadKey:
      case InputEventType_GamePadJoystick:
      default:
        break;
    }
  }
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

void InputManager::readMousePositionEvent(CircularBuffer& buf) {
  MousePositionEvent e;
  buf.read(&e);
  currMousePos.x = e.xpos;
  currMousePos.y = e.ypos;

	mouseMode = MouseMode::Normal;
  if (e.hidden)
    mouseMode = MouseMode::Hidden;
  if (e.disabled)
    mouseMode = MouseMode::Disabled;
}
