#pragma once

#include <GLFW/glfw3.h>

#include <array>
#include <bitset>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

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

   private:

    InputManager* _host;
  };
		
 public:
	 InputManager() { _conn
	}

  void process() {
    std::bitset<kGLFWNumKeys> pressed{}, released{};

    pressed = currKeys & (~prevKeys);
    released = (~currKeys) & (prevKeys);

    frameCount++;
  }

  // Register Controller callbacks.
  

  struct KeyBinding {
    void remove() {
      if (_conn.isConnected()) _conn.getHost()->remove(id);
    }

   private:
    uint32_t id{0};
    std::shared_ptr<Connector> _conn;
  };
	
	template<auto Method, typename T>
  KeyBinding bindKey(T* obj, int key, KeyActionType action, int priority = 0, bool consume = false) {
		
  }

 private:
  std::bitset<kGLFWNumKeys> prevKeys{}, currKeys{};

  std::bitset<kGLFWNumMouseButtons> prevMouseButtons{}, currMouseButtons{};
  glm::vec2 prevMousePos{}, currMousePos{};
  glm::vec2 mouseDelta{};

  size_t frameCount{0};
  std::array<std::vector<uint8_t>, 2> eventQueues{std::vector<uint8_t>(4096),
                                                  std::vector<uint8_t>(4096)};

  size_t getCurrQueueIndex() const { return frameCount % eventQueues.size(); }
  std::vector<uint8_t>& getCurrEventQueue() {
    return eventQueue[currQueueIndex];
  }

 private:
  struct KeyBindingSlot {
    uint32_t id;
    Delegate<void()> d;
		int key;
		KeyActionType action;
		int priority;
		bool consume;
		bool valid;
  };

  std::shared_ptr<Connection> _conn;

  std::vector<KeyBindingSlot> keyBindingSlots;
  KeyBinding addKeyBindingSlot(Delegate<void()> d, int key,
                               KeyActionType action, int priority,
                               bool consume) {
    uint32_t id = keyBindingSlots
                      .size() + 1;
	
    keyBindingSlots.emplace_back(id,
                    std::move(d),
                    key,
                    action,
                    priority,
                    consume,
                    true);
    std::stable_sort(keyBindingSlots.begin(), keyBindingSlots.end(),
                     [](auto& a, auto& b) {return a.priority > b.priority;}
		)

		return KeyBinding{id, _conn};
	}

  void removeKeyBinding(uint32_t id) {
		
	}
};

class Controller {};
