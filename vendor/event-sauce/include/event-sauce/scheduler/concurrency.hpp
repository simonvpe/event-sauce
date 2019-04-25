#pragma once

namespace event_sauce::concurrency {
struct parallel_tag
{};

struct serialized_tag
{};

static inline constexpr auto parallel = parallel_tag{};
static inline constexpr auto serialized = serialized_tag{};
} // event_sauce::concurrency
