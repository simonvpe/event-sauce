#pragma once
#include <event-sauce/scheduler/concurrency.hpp>

namespace event_sauce {
struct default_scheduler_type
{
  template<typename Fn>
  struct future
  {
    Fn fn;
    future(Fn&& fn)
        : fn{ std::forward<Fn>(fn) }
    {}
    auto get() const
    {
      return fn();
    }
  };

  auto operator()(concurrency::parallel_tag)
  {
    return [](auto&& fn) { return future{ std::forward<Fn>(fn) }; };
  }

  auto operator()(concurrency::serialized_tag)
  {
    return [](auto&& fn) { return fn(); };
  }
};

}
