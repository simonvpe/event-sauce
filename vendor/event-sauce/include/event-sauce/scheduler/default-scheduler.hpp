#pragma once
#include <event-sauce/scheduler/concurrency.hpp>

namespace event_sauce {
struct default_scheduler_type
{
  auto operator()(concurrency::parallel_tag)
  {
    return [](auto&& fn) { fn(); };
  }

  auto operator()(concurrency::serialized_tag)
  {
    return [](auto&& fn) { fn(); };
  }
};

}
