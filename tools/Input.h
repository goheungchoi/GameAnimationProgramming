#pragma once

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "CircularBuffer.h"
#include "Delegate.h"

constexpr size_t kGLFWNumKeys = GLFW_KEY_LAST + 1;
constexpr size_t kGLFWNumMouseButtons = GLFW_MOUSE_BUTTON_LAST + 1;

using DeviceId = uint64_t;

enum InputEventType : uint32_t {
  InputEventType_Unknown,
  InputEventType_Keyboard,
  InputEventType_MouseButton,
  InputEventType_MousePosition,
  InputEventType_GamePadKey,
  InputEventType_GamePadJoystick
};

struct InputEventHeader {
  InputEventType type;
};

struct KeyEvent {
  InputEventType type;

  int key;
  int scancode;
  int action;

  bool shift;
  bool ctrl;
  bool alt;
};

struct MouseButtonEvent {
  InputEventType type;

  int button;
  int action;

  bool shift;
  bool ctrl;
  bool alt;
};

struct MousePositionEvent {
  InputEventType type;

  int xpos;
  int ypos;
};

enum class KeyActionType {
  Pressed,
  Released,
  Down,
};

class InputManager {
  struct Connection {
    bool isConnected() { return !_host; }

    InputManager* getHost() { return _host; }

    InputManager* _host;
  };

  struct KeyBindingSlot {
    uint32_t id;
    Delegate<void()> d;
    int key;
    KeyActionType action;
    int priority;
    bool consume;
    bool valid;
  };

 public:
  InputManager() : _conn{std::make_shared<Connection>(this)} {}

  // Call this once per frame after updating currKeys from your backend
  void process() {
    pollEvents();

    processKeyBindings();
  }

  void pushKeyEvent(const KeyEvent& e) {
    std::lock_guard<std::mutex> lock(getCurrEventQueueMutex());

    CircularBuffer& buf = getCurrEventQueue();
    buf.write<KeyEvent>(&e);
  }

  struct KeyBinding {
    void remove() {
      if (_conn && _conn->isConnected()) {
        _conn->getHost()->removeKeyBindingSlot(id);
      }
    }

    uint32_t id;
    std::shared_ptr<Connection> _conn;
  };

  KeyBinding bindKey(Delegate<void()> d, int key, KeyActionType action,
                     int priority = 0,
                     bool consume = false) {
    return addKeyBindingSlot(std::move(d), key, action,
                             priority, consume);
  }

 private:
  std::bitset<kGLFWNumKeys> prevKeys{}, currKeys{};

  std::bitset<kGLFWNumMouseButtons> prevMouseButtons{}, currMouseButtons{};
  glm::vec2 prevMousePos{}, currMousePos{};
  glm::vec2 mouseDelta{};

  size_t indexCount{0};
  std::array<CircularBuffer, 2> eventQueues{};
  std::array<std::mutex, 2> eventQueueMutexes{};

  size_t getCurrQueueIndex() const { return indexCount % eventQueues.size(); }
  CircularBuffer& getCurrEventQueue() {
    return eventQueues[getCurrQueueIndex()];
  }
  std::mutex& getCurrEventQueueMutex() {
    return eventQueueMutexes[getCurrQueueIndex()];
	}

	void pollEvents() {
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
				default: break;
      }
    }

		++indexCount;
	}

	void readKeyEvent(CircularBuffer& buf) {
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

 private:
  std::shared_ptr<Connection> _conn;

  // ===== Slot storage (ID == index) =====
  std::vector<KeyBindingSlot> keyBindingSlots;        // dense vector
  std::vector<uint32_t> freeKeyBindingIndices;        // LIFO free-list
  std::vector<uint32_t> keyBindingSlotDeletionQueue;  // deferred removes
  std::vector<uint32_t> tempActive_;  // frame-local list of indices

  // Allocate a slot index (reuse hole or grow)
  uint32_t allocSlotIndex_() {
    if (!freeKeyBindingIndices.empty()) {
      uint32_t idx = freeKeyBindingIndices.back();
      freeKeyBindingIndices.pop_back();
      return idx;
    }
    keyBindingSlots.emplace_back();  // default slot at tail
    return static_cast<uint32_t>(keyBindingSlots.size() - 1);
  }

  // Core add function
  KeyBinding addKeyBindingSlot(Delegate<void()> d, int key,
                                KeyActionType action, int priority,
                                bool consume) {
    uint32_t idx = allocSlotIndex_();
    KeyBindingSlot& slot = keyBindingSlots[idx];
    slot.id = idx;
    slot.d = std::move(d);
    slot.key = key;
    slot.action = action;
    slot.priority = priority;
    slot.consume = consume;
    slot.valid = true;

    KeyBinding hb{idx, _conn};
    return hb;
  }

	void removeKeyBindingSlot(uint32_t id) { keyBindingSlotDeletionQueue.push_back(id); }

  // Process deferred removals
  void flushPendingRemovals() {
    if (keyBindingSlotDeletionQueue.empty()) return;
    for (uint32_t id : keyBindingSlotDeletionQueue) {
      if (id < keyBindingSlots.size()) {
        auto& slot = keyBindingSlots[id];
        if (slot.valid) {
          // clear and recycle
          slot.valid = false;
          slot.d = Delegate<void()>{};
          slot.key = -1;
          slot.priority = 0;
          slot.consume = false;
          freeKeyBindingIndices.push_back(id);
        }
      }
    }
    keyBindingSlotDeletionQueue.clear();
  }

  void processKeyBindings() {
    // finalize any deferred removals from last frame
    flushPendingRemovals();

    std::bitset<kGLFWNumKeys> pressed{};
    std::bitset<kGLFWNumKeys> released{};
    std::bitset<kGLFWNumKeys> consumed{};

    pressed = currKeys & (~prevKeys);
    released = (~currKeys) & (prevKeys);

    // Build a working list of active slot indices
    tempActive_.clear();
    tempActive_.reserve(keyBindingSlots.size());
    for (uint32_t idx = 0; idx < keyBindingSlots.size(); ++idx) {
      const auto& slot = keyBindingSlots[idx];
      if (slot.valid && slot.key >= 0 &&
          slot.key < static_cast<int>(kGLFWNumKeys)) {
        tempActive_.push_back(idx);
      }
    }

    // Sort *indices* by priority (desc), tie-breaker: earlier id first
    std::stable_sort(tempActive_.begin(), tempActive_.end(),
                     [this](uint32_t a, uint32_t b) {
                       const auto& sa = keyBindingSlots[a];
                       const auto& sb = keyBindingSlots[b];
                       if (sa.priority != sb.priority)
                         return sa.priority > sb.priority;
                       return sa.id < sb.id;
                     });

    // Dispatch in priority order, honoring consume
    for (uint32_t idx : tempActive_) {
      const auto& slot = keyBindingSlots[idx];
      const int k = slot.key;
      if (consumed.test(k)) continue;

      bool trigger = false;
      switch (slot.action) {
        case KeyActionType::Pressed:
          trigger = pressed.test(k);
          break;
        case KeyActionType::Released:
          trigger = released.test(k);
          break;
        case KeyActionType::Down:
          trigger = currKeys.test(k);
          break;
      }

      if (trigger && slot.d) {
        slot.d();  // invoke
        if (slot.consume) consumed.set(k);
      }
    }

    prevKeys = currKeys;
  }
};

class Controller {};
