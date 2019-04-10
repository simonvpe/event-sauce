#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <event-sauce/event-sauce.hpp>

struct Model
{};

struct Aggregate
{
  struct Increment
  {
    int value = 0;
  };

  struct Incremented
  {
    int value = 0;
  };

  struct state_type
  {
    int value = 0;
  };

  static constexpr Incremented execute(const state_type&, const Increment& cmd) { return { cmd.value }; }

  static constexpr state_type apply(const state_type& state, const Incremented& evt)
  {
    return { state.value + evt.value };
  }
};

TEST_SUITE("simple event dispatching")
{
  SCENARIO("increment counter")
  {
    GIVEN("a counter context")
    {
        auto ctx = event_sauce::make_context<Aggregate>();
      WHEN("dispatching a single increment command")
      {
        auto dispatch = event_sauce::dispatch(ctx);        
        dispatch(Aggregate::Increment{ 42 });
        THEN("the state should change") { CHECK(ctx.inspect<Aggregate>().value == 42); }
      }
      WHEN("dispatching several increment commands")
      {
        auto dispatch = event_sauce::dispatch(ctx);
        dispatch(Aggregate::Increment{ 25 });
        dispatch(Aggregate::Increment{ 50 });
        THEN("the resulting state should reflect the increments") { CHECK(ctx.inspect<Aggregate>().value == 75); }
      }
      WHEN("dispatching a single increment command")
      {
        auto counter = 0;
        auto projector = [&](const Aggregate::Incremented& evt) { counter += evt.value; };
        auto dispatch = event_sauce::dispatch(ctx, projector);
        dispatch(Aggregate::Increment{100});
        THEN("the projection should be called") { CHECK(counter == 100); }
      }
    }
  }
}

