#pragma once

#include <type_traits>
#include <utility>

template <typename>
struct Delegate;

template <typename Ret, typename... Args>
struct Delegate<Ret(Args...)> {
  using Stub = Ret (*)(void*, Args...);
  using Destroy = void (*)(void*);
  using Clone = void* (*)(void*);

  Delegate() = default;
  ~Delegate() { reset(); }

  Delegate(const Delegate& other)
      : obj(other.obj),
        stub(other.stub),
        destroy(other.destroy),
        clone(other.clone) {
    if (other.obj && clone) obj = clone(other.obj);
  }

  Delegate(Delegate&& other) noexcept
      : obj(other.obj),
        stub(other.stub),
        destroy(other.destroy),
        clone(other.clone) {
    other.obj = nullptr;
    other.stub = nullptr;
    other.destroy = nullptr;
    other.clone = nullptr;
  }

  Delegate& operator=(const Delegate& other) {
    if (this == &other) return *this;
    reset();
    obj = other.obj;
    stub = other.stub;
    destroy = other.destroy;
    clone = other.clone;
    if (other.obj && clone) obj = clone(other.obj);
    return *this;
  }

  Delegate& operator=(Delegate&& rhs) noexcept {
    if (this == &rhs) return *this;
    reset();
    obj = rhs.obj;
    stub = rhs.stub;
    destroy = rhs.destroy;
    clone = rhs.clone;
    rhs.obj = nullptr;
    rhs.stub = nullptr;
    rhs.destroy = nullptr;
    rhs.clone = nullptr;
    return *this;
  }

  void reset() {
    if (destroy && obj) destroy(obj);
    obj = nullptr;
    stub = nullptr;
    destroy = nullptr;
    clone = nullptr;
  }

  // --- binders ---

  // Member function (non-owning)
  template <class T, Ret (T::*Method)(Args...)>
  static Delegate bind(T* instance) {
    static_assert(std::is_member_function_pointer_v<decltype(Method)>);
    Delegate d;
    d.obj = instance;
    d.stub = [](void* o, Args... a) -> Ret {
      return (static_cast<T*>(o)->*Method)(std::forward<Args>(a)...);
    };
    return d;
  }

  // Free function (non-owning)
  static Delegate bind(Ret (*func)(Args...)) {
    Delegate d;
    d.obj = reinterpret_cast<void*>(func);
    d.stub = [](void* f, Args... a) -> Ret {
      auto fn = reinterpret_cast<Ret (*)(Args...)>(f);
      return fn(std::forward<Args>(a)...);
    };
    return d;
  }

  // Functor pointer (non-owning)
  template <class Functor>
  static Delegate bind(Functor* fptr) {
    Delegate d;
    d.obj = fptr;
    d.stub = [](void* o, Args... a) -> Ret {
      return (*static_cast<Functor*>(o))(std::forward<Args>(a)...);
    };
    return d;
  }

  // Functor value (owning)
  template <class Functor>
  static Delegate bind(Functor&& f) {
    using F = std::decay_t<Functor>;
    auto* heap = new F(std::forward<Functor>(f));
    Delegate d;
    d.obj = heap;
    d.stub = [](void* o, Args... a) -> Ret {
      return (*static_cast<F*>(o))(std::forward<Args>(a)...);
    };
    d.destroy = +[](void* o) { delete static_cast<F*>(o); };
    d.clone = +[](void* o) -> void* { return new F(*static_cast<F*>(o)); };
    return d;
  }

  // call
  Ret operator()(Args... a) const {
    return stub(obj, std::forward<Args>(a)...);
  }

  explicit operator bool() const { return stub != nullptr; }

  bool same_target(const Delegate& other) const {
    return obj == other.obj && stub == other.stub;
  }

 private:
  void* obj{nullptr};
  Stub stub{nullptr};
  Destroy destroy{nullptr};
  Clone clone{nullptr};
};
