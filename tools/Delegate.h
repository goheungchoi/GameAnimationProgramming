#pragma once

#include <type_traits>
#include <utility>

template <typename>
struct Delegate;

template <typename Ret, typename... Args>
struct Delegate<Ret(Args...)> {
  using Stub = Ret (*)(void*, Args...);

  Delegate() noexcept = default;

  template <auto Method, typename T>
  static Delegate bind(T* instance) {
    static_assert(std::is_member_function_pointer_v<decltype(Method)>);
    static_assert(std::is_invocable_r_v<Ret, decltype(Method), T&, Args...>);
    Delegate d;
    d.obj = instance;
    d.stub = +[](void* o, Args... a) -> Ret {
      T* obj = static_cast<T*>(o);
      return std::invoke(Method, *obj, std::forward<Args>(a)...);
    };
    return d;
  }

  static Delegate bind(Ret (*func)(Args...)) {
    Delegate d;
    d.obj = reinterpret_cast<void*>(func);
    d.stub = +[](void* f, Args... a) -> Ret {
      auto fn = reinterpret_cast<Ret (*)(Args...)>(f);
      return std::invoke(fn, std::forward<Args>(a)...); 
    };
    return d;
  }

  template <class Functor>
  static Delegate bind(Functor* fptr) {
    Delegate d;
    d.obj = fptr;
    d.stub = +[](void* o, Args... a) -> Ret {
      return (*static_cast<Functor*>(o))(std::forward<Args>(a)...);
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
};
