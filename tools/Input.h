#pragma once

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <functional>
#include <glm/vec2.hpp>
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

  float xpos;
  float ypos;

  bool hidden;
  bool disabled;
};

enum class KeyActionType {
  Pressed,
  Released,
  Down,
};

enum class MouseMode {
  Normal,
  Hidden,
  Disabled,
  Any,
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

  struct MouseMoveBindingSlot {
    uint32_t id;
    bool valid;

    Delegate<void(float, float)> d;
    MouseMode mode;
    int priority;
    bool consume;
  };

  // Binding slot table to ease the management of slot ids and empty spots.
  template <typename T>
  struct SlotTable {
    // ===== Slot storage (ID == index) =====
    std::vector<T> bindingSlots;                     // dense vector
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
    uint32_t addBindingSlot(Args&&... args) {
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

    // Returns active slot lists sorted by priority level.
    // Any slot that satisfies the condition considers to be active.
    std::vector<uint32_t> getActiveSlotIndices(
        std::function<bool(const T&)> cond) {
      // Build a working list of active slot indices
      std::vector<uint32_t> tempActive_;  // frame-local list of indices
      tempActive_.reserve(getNumBindingSlots());
      for (uint32_t idx = 0; idx < getNumBindingSlots(); ++idx) {
        const auto& slot = bindingSlots[idx];
        if (cond(slot)) {
          tempActive_.push_back(idx);
        }
      }

      // Sort *indices* by priority (desc), tie-breaker: earlier id first
      std::stable_sort(tempActive_.begin(), tempActive_.end(),
                       [this](uint32_t a, uint32_t b) {
                         const auto& sa = bindingSlots[a];
                         const auto& sb = bindingSlots[b];
                         if (sa.priority != sb.priority)
                           return sa.priority > sb.priority;
                         return sa.id < sb.id;
                       });
      return tempActive_;
    }

    T& operator[](size_t idx) { return bindingSlots[idx]; }
    const T& operator[](size_t idx) const { return bindingSlots[idx]; }

   private:
  };

  // Slot tables.
  SlotTable<KeyBindingSlot> keyBindingSlots;
  SlotTable<MouseMoveBindingSlot> mouseMoveBindingSlots;

 public:
  InputManager() : _conn{std::make_shared<Connection>(this)} {}

  // Call this once per frame after updating currKeys from your backend
  void process();

  void pushKeyEvent(const KeyEvent& e);
  void pushMousePositionEvent(const MousePositionEvent& e);

  struct KeyBinding {
    uint32_t id;
    std::shared_ptr<Connection> _conn;

    void remove();
  };
  KeyBinding bindKey(Delegate<void()> d, int key, KeyActionType action,
                     int priority = 0, bool consume = false);

  struct MouseMoveBinding {
    uint32_t id;
    std::shared_ptr<Connection> _conn;

    void remove();
  };
  MouseMoveBinding bindMouseMove(Delegate<void(float, float)> d,
                                 MouseMode mode = MouseMode::Any,
                                 int priority = 0, bool consume = false);

 private:
  std::bitset<kGLFWNumKeys> prevKeys{}, currKeys{};

  std::bitset<kGLFWNumMouseButtons> prevMouseButtons{}, currMouseButtons{};

  bool bHasInitMousePos{};
  MouseMode mouseMode{};
  glm::vec2 prevMousePos{}, currMousePos{};

  std::atomic<size_t> writeIndex{0};
  std::array<CircularBuffer, 2> eventQueues{};
  std::array<std::mutex, 2> queueMutexes{};

  size_t getWriteQueueIndex() const {
    return writeIndex.load(std::memory_order_acquire);
  }
  CircularBuffer& getCurrEventQueue() {
    return eventQueues[getWriteQueueIndex()];
  }

  void pollEvents();
  void readKeyEvent(CircularBuffer& buf);
  void readMousePositionEvent(CircularBuffer& buf);

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

    // Get a working list of active slot indices
    auto activeSlotIndicies =
        keyBindingSlots.getActiveSlotIndices([&](const KeyBindingSlot& slot) {
          return slot.valid && slot.key >= 0 &&
                 slot.key < static_cast<int>(kGLFWNumKeys);
        });

    // Dispatch in priority order, honoring consume
    for (uint32_t idx : activeSlotIndicies) {
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

  void processMouseMoveBindings() {
    mouseMoveBindingSlots.flushPendingRemovals();

    glm::vec2 mouseDelta{};
    if (!bHasInitMousePos) {
      prevMousePos = currMousePos;
      bHasInitMousePos = true;
    }
    mouseDelta = currMousePos - prevMousePos;

    // Check if there was a mouse move.
    if (mouseDelta.length() <= std::numeric_limits<float>::epsilon()) {
      return;
    }

    // Get a working list of active slot indices
    auto activeSlotIndicies = mouseMoveBindingSlots.getActiveSlotIndices(
        [&](const MouseMoveBindingSlot& slot) { return slot.valid; });

    // Dispatch in priority order, honoring consume
    for (uint32_t idx : activeSlotIndicies) {
      const auto& slot = mouseMoveBindingSlots[idx];

      bool trigger = (mouseMode == slot.mode);
      if (slot.mode == MouseMode::Any) trigger = true;

      if (trigger && slot.d) {
        slot.d(mouseDelta.x, mouseDelta.y);  // invoke
        if (slot.consume) break;
      }
    }

    prevMousePos = currMousePos;
  }
};

class Controller {};
