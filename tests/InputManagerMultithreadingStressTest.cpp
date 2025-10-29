// InputManagerSPSCDeterministic.cpp
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

#include "Input.h"  // your InputManager

using namespace std::chrono;

int main() {
  constexpr int TARGET_FPS = 60;
  constexpr int TOTAL_FRAMES = 600;     // ~10 seconds at 60 FPS
  constexpr int TEST_KEY = GLFW_KEY_A;  // single key for deterministic toggling

  InputManager input;

  // ===== Metrics =====
  std::atomic<uint64_t> cb_press{0};
  std::atomic<uint64_t> cb_release{0};

  // ===== Frame barrier state (deterministic handshake) =====
  std::mutex m;
  std::condition_variable cv_prod;  // consumer -> producer (new frame ready)
  std::condition_variable cv_cons;  // producer -> consumer (events pushed)
  int frame_request = -1;           // frame index the consumer wants produced
  int frame_done = -1;              // last frame index producer finished
  bool stop = false;

  // ===== Producer: push exactly one deterministic event per frame =====
  auto producer_fn = [&]() {
    std::unique_lock<std::mutex> lk(m);
    while (!stop) {
      // Wait for the consumer to request the next frame
      cv_prod.wait(lk, [&] { return stop || frame_request == frame_done + 1; });
      if (stop) break;

      const int f = frame_request;
      lk.unlock();  // release lock while pushing to avoid blocking the consumer

      // Deterministic pattern:
      //   even frame -> PRESS, odd frame -> RELEASE
      KeyEvent e{};
      e.type = InputEventType_Keyboard;
      e.key = TEST_KEY;
      e.scancode = 0;
      e.action = (f % 2 == 0) ? GLFW_PRESS : GLFW_RELEASE;
      e.shift = false;
      e.ctrl = false;
      e.alt = false;

      input.pushKeyEvent(e);

      lk.lock();
      frame_done = f;
      cv_cons.notify_one();  // signal consumer that this frame's event is ready
    }
  };

  std::thread producer(producer_fn);

  // ===== Consumer: main/game thread @ 60 FPS =====
  auto t0 = steady_clock::now();
  const auto frame_dt = duration<double>(1.0 / TARGET_FPS);

  for (int frame = 0; frame < TOTAL_FRAMES; ++frame) {
    auto frame_start = steady_clock::now();

    // 1) Tell producer which frame to produce and wait until it’s done
    {
      std::unique_lock<std::mutex> lk(m);
      frame_request = frame;
      cv_prod.notify_one();
      cv_cons.wait(lk, [&] { return frame_done == frame; });
    }

    // 2) Process exactly once this frame (deterministic)
    input.process();

    // 3) Sleep to hit 60 FPS cadence (fixed interval)
    const auto next =
        frame_start + duration_cast<steady_clock::duration>(frame_dt);
    std::this_thread::sleep_until(next);
  }

  // Stop producer and join
  {
    std::lock_guard<std::mutex> lk(m);
    stop = true;
    cv_prod.notify_one();
  }
  producer.join();

  // Final drain to catch leftovers (should be none)
  input.process();

  auto t1 = steady_clock::now();
  const double secs = duration<double>(t1 - t0).count();

  // ===== Report (deterministic expectations) =====
  const uint64_t callbacks_press = cb_press.load(std::memory_order_relaxed);
  const uint64_t callbacks_rel = cb_release.load(std::memory_order_relaxed);
  const uint64_t cb_total = callbacks_press + callbacks_rel;

  // Expected numbers:
  //  - We produced exactly one event per frame:
  //      PRESS on even frames:  ceil(TOTAL_FRAMES / 2)
  //      RELEASE on odd frames: floor(TOTAL_FRAMES / 2)
  const uint64_t expected_press = (TOTAL_FRAMES + 1) / 2;  // even count
  const uint64_t expected_release = TOTAL_FRAMES / 2;      // odd count

  std::cout << "===== Deterministic SPSC Input Test (60 FPS) =====\n";
  std::cout << "Frames: " << TOTAL_FRAMES << " (~" << secs << " s @ "
            << TARGET_FPS << " FPS)\n";
  std::cout << "Key: " << TEST_KEY << "\n\n";

  std::cout << "Expected  Press callbacks: " << expected_press << "\n";
  std::cout << "Observed  Press callbacks: " << callbacks_press << "\n";
  std::cout << "Expected  Release callbacks: " << expected_release << "\n";
  std::cout << "Observed  Release callbacks: " << callbacks_rel << "\n\n";

  std::cout << "Total callbacks: " << cb_total << "  (press=" << callbacks_press
            << ", release=" << callbacks_rel << ")\n";
  std::cout << "Avg callback rate: " << (cb_total / secs) << " invokes/s\n";
  std::cout << "==================================================\n";

  return 0;
}
