#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <typeinfo>
#include <utility>

template <typename>
typename Delegate;

template <typename Ret, typename... Args>
struct Delegate<Ret(Args...)> {
  using Stub = void (*)(void*, Args&&...);
  using Destory = void (*)(void*);
  using Clone = void* (*)(void*);

  Delegate() = default;

  ~Delegate() { reset(); }

  Delegate(const Delegate& other)
      : stub(other.stub), destroy(other.destroy), clone(other.clone) {
    if (other.obj) {
      obj = clone ? clone(other.obj) : other.obj;
    }
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
    stub = other.stub;
    destroy = other.destroy;
    clone = other.clone;
    if (other.obj) {
      obj = clone ? clone(other.obj) : other.obj;
    }
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

  template <auto Method, class T>
  static Delegate bind(T* instance) {
    static_assert(std::is_member_function_pointer_v<decltype(Method)>);
    Delegate d;
    d.obj = instance;
    d.stub = [](void* o, Args&&... a) -> Ret {
      return (static_cast<T*>(o)->*Method)(std::forward<Args>(a)...);
    };
    return d;
  }

  static Delegate bind(void (*func)(Args...)) {
    Delegate d;
    d.obj = reinterpret_cast<void*>(func);
    d.stub = [](void* f, Args&&... a) -> Ret {
      auto fn = reinterpret_cast<void (*)(Args...)>(f);
      return fn(std::forward<Args>(a)...);
    };

    return d;
  }

  template <class Functor>
  static Delegate bind(Functor* fptr) {
    Delegate d;
    d.obj = fptr;
    d.stub = [](void* o, Args&&... a) -> Ret {
      return (*static_cast<Functor*>(o))(std::forward<Args>(a)...);
    };
    return d;
  }

  template <class Functor>
  static Delegate bind(Functor f) {
    auto* o = new Functor(std::move(f));
    Delegate d;
    d.obj = o;
    d.stub = [](void* o, Args&&... a) -> Ret {
      return (*static_cast<Functor*>(o))(std::forward<Args>(a)...);
    };
    d.destroy = +[](void* o) { delete static_cast<Functor*>(o); };
    d.clone = [](void* o) -> void* {
      return new Functor(*static_cast<Functor*>(o));
    };
    return d;
  }

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
  Destory destroy{nullptr};
  Clone clone{nullptr};

  std::unique_ptr<char> storage{nullptr};
};
