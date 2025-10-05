#pragma once

#include <GLFW/glfw3.h>

#include <glm/vec2.hpp>

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

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
    bool valid;

    Delegate<void()> d;
    int key;
    KeyActionType action;
    int priority;
    bool consume;
  };

	template<typename T>
  struct SlotTable {
    // ===== Slot storage (ID == index) =====
    std::vector<T> bindingSlots;        // dense vector
    std::vector<uint32_t> freeBindingIndices;        // LIFO free-list
    std::vector<uint32_t> bindingSlotDeletionQueue;  // deferred removes

    size_t getNumBindingSlots() const { return bindingSlots.size(); }

    // Allocate a slot index (reuse hole or grow)
    uint32_t allocSlotIndex_() {
      if (!freeBindingIndices.empty()) {
        uint32_t idx = freeBindingIndices.back();
        freeBindingIndices.pop_back();
        return idx;
      }
      bindingSlots.emplace_back();  // default slot at tail
      return static_cast<uint32_t>(bindingSlots.size() - 1);
    }

    // Core add function
    template <typename... Args>
    uint32_t addKeyBindingSlot(Args&&... args) {
      uint32_t idx = allocSlotIndex_();
      bindingSlots[idx] = T{idx, true, std::forward<Args>(args)...};
			return idx;
    }

    void removeKeyBindingSlot(uint32_t id) {
      bindingSlotDeletionQueue.push_back(id);
    }

    // Process deferred removals
    void flushPendingRemovals() {
      if (bindingSlotDeletionQueue.empty()) return;
      for (uint32_t id : bindingSlotDeletionQueue) {
        if (id < bindingSlots.size()) {
          auto& slot = bindingSlots[id];
          if (slot.valid) {
            // clear and recycle
            slot = T{};
            freeBindingIndices.push_back(id);
          }
        }
      }
      bindingSlotDeletionQueue.clear();
    }

		T& operator[](size_t idx) { return bindingSlots[idx]; }
    const T& operator[](size_t idx) const { return bindingSlots[idx]; }
  };

  SlotTable<KeyBindingSlot> keyBindingSlots;

 public:
  InputManager() : _conn{std::make_shared<Connection>(this)} {}

  // Call this once per frame after updating currKeys from your backend
  void process();

  void pushKeyEvent(const KeyEvent& e);

  struct KeyBinding {
    uint32_t id;
    std::shared_ptr<Connection> _conn;

    void remove();
  };
  KeyBinding bindKey(Delegate<void()> d, int key, KeyActionType action,
                     int priority = 0, bool consume = false);

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

  void pollEvents();
  void readKeyEvent(CircularBuffer& buf);

 private:
  std::shared_ptr<Connection> _conn;

  void processKeyBindings() {
    // finalize any deferred removals from last frame
    keyBindingSlots.flushPendingRemovals();

    std::bitset<kGLFWNumKeys> pressed{};
    std::bitset<kGLFWNumKeys> released{};
    std::bitset<kGLFWNumKeys> consumed{};

    pressed = currKeys & (~prevKeys);
    released = (~currKeys) & (prevKeys);

    // Build a working list of active slot indices
    std::vector<uint32_t> tempActive_;  // frame-local list of indices
    tempActive_.reserve(keyBindingSlots.getNumBindingSlots());
    for (uint32_t idx = 0; idx < keyBindingSlots.getNumBindingSlots(); ++idx) {
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
