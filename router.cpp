#include <iostream>
#include <tuple>
#include <variant>

/*******************************************************************************
 ** Router
 **
 ** Routes events to subscribers.
 *******************************************************************************/
namespace router_detail {
template <class... Us> struct Overloaded : Us... {
  using Us::operator()...;
  template <typename U> void operator()(const U &) const {}
};
template <class... Us> Overloaded(Us...)->Overloaded<Us...>;

template <typename... Ts> struct RouterImpl {
private:
  std::tuple<Ts...> subscribers;

  struct Visitor {
    Visitor(RouterImpl &router) : router{router} {}
    RouterImpl &router;

    template <typename T> void operator()(const T &value) {
      router.publish(value);
    }
  };

public:
  RouterImpl(Ts &&... ts) : subscribers{std::move(ts)...} {}

  template <typename... Fs>
  RouterImpl<Ts..., router_detail::Overloaded<Fs...>> subscribe(Fs &&... fs) {
    return {std::move(std::get<Ts>(subscribers))...,
            router_detail::Overloaded{std::move(fs)...}};
  }

  template <typename T> void publish(const T &evt) {
    std::cout << "(5) publishing event " << typeid(evt).name() << std::endl;
    std::cout << "(6) subscribers are " << typeid(subscribers).name()
              << std::endl;
    auto wrap = [&evt](const auto &sub) {
      std::cout << "(7) wrap called for sub" << typeid(sub).name() << std::endl;
      sub.apply(evt, 0);
      return 0;
    };

    std::apply([&wrap](auto &&... xs) { return std::make_tuple(wrap(xs)...); },
               subscribers);
  }

  template <typename... Us> void publish(std::variant<Us...> &&v) {
    std::cout << "(4) publishing variant" << std::endl;
    std::visit(Visitor(*this), v);
  }
};
} // namespace router_detail

struct Router : public router_detail::RouterImpl<> {
  Router() {}
};

Router make_router() { return {}; }

/*******************************************************************************
 ** Context
 *******************************************************************************/

template <typename Aggregate> struct AggregateDispatch {
  using state_type = typename Aggregate::state_type;
  state_type const &state;
  AggregateDispatch(const state_type &state) : state{state} {}

  template <typename Event>
  using can_apply_type = decltype(Aggregate::apply(
      std::declval<const state_type &>(), std::declval<const Event &>()));

  template <typename Event>
  auto apply(const Event &evt, int) const -> can_apply_type<Event> {
    std::cout << "aggregate dispatch matches " << typeid(evt).name()
              << std::endl;
    Aggregate::apply(state, evt);
    return state;
  }

  template <typename Event> state_type apply(const Event &evt, long) const {
    std::cout << "aggregate dispatch no match" << std::endl;
    return state;
  }

  // I have no idea why operator() behaves differently than a 'normal' function
  template <typename Event>
  auto operator()(const Event &e) const -> can_apply_type<Event> {
    std::cout << "aggregate dispatch matches " << typeid(e).name() << std::endl;
    return Aggregate::apply(state, e);
  }

  template <typename Event> state_type operator()(const Event &) const {
    std::cout << "aggregate dispatch no match" << std::endl;
    return state;
  }
};

template <typename State, typename Aggregate>
auto bind(const State &state, Aggregate &&aggregate) {
  auto &s = std::get<typename Aggregate::state_type>(state);
  return std::move(make_router().subscribe(AggregateDispatch<Aggregate>{s}));
}

template <typename... Aggregates> struct Context {
  using state_type = std::tuple<typename Aggregates::state_type...>;
  using router_type = decltype(
      bind(std::declval<state_type &>(), std::declval<Aggregates>()...));

  state_type state;
  router_type router;

  Context() : state{}, router{bind(state, Aggregates{}...)} {}

  template <typename Command> void dispatch(const Command &cmd) {
    std::cout << "(1) router is " << typeid(router).name() << std::endl;
    auto events = std::make_tuple(Aggregates::execute(
        std::get<typename Aggregates::state_type>(state), cmd)...);

    auto wrap = [this](auto &&evt) {
      std::cout << "(3) publishing " << typeid(evt).name() << std::endl;
      this->router.publish(std::move(evt));
      return 0;
    };

    std::apply(
        [&wrap](auto &&... xs) {
          return std::make_tuple(wrap(std::move(xs))...);
        },
        events);
  }
};

// Public event, everyone knows about it

struct E1 {};
struct E2 {};
struct E3 {};

struct C1 {
  int x, y;
};

struct C2 {
  int x, y;
};

struct Aggregate {
  struct state_type {
    int x, y;
  };

  static std::variant<E1, E2, E3> execute(const state_type &state,
                                          const C1 &command) {
    std::cout << "(2) AGGREGATE C1" << std::endl;
    return E1{};
  }

  static std::variant<E1, E2, E3> execute(const state_type &state,
                                          const C2 &command) {
    std::cout << "(2) AGGREGATE C2" << std::endl;
    return E3{};
  }

  static state_type apply(const state_type &state, const E1 &event) {
    std::cout << "AGGREGATE E1" << std::endl;
    return state;
  }

  static state_type apply(const state_type &state, const E3 &event) {
    std::cout << "AGGREGATE E3" << std::endl;
    return state;
  }
};

int main() {
  auto ctx = Context<Aggregate>{};
  ctx.dispatch(C1{12, 13});
  ctx.dispatch(C2{24, 25});
  //  auto ctx = bind_aggregate<Aggregate>(router);

  //  ctx.dispatch(C1{12, 13});
  return 0;
}
