#include "aggregates/player.hpp"
#include "aggregates/time.hpp"
#include "networking/client.hpp"
#include "networking/router.hpp"
#include "vendor/event-sauce.hpp"
#include <future>
#include <immer/map.hpp>
#include <iostream>

struct ServerAggregate
{
  struct Speak
  {
    std::string sentence;
  };

  struct Spoke
  {
    std::string sentence;
  };

  struct state_type
  {};

  static Spoke execute(const state_type& state, const Speak& cmd) { return { cmd.sentence }; }

  template<typename Event>
  static auto project(const Event& evt);
};

struct ClientAggregate
{
  struct Speak
  {
    std::string sentence;
  };

  struct Spoke
  {
    std::string sentence;
  };

  struct state_type
  {};

  static Spoke execute(const state_type& state, const Speak& cmd) { return { cmd.sentence }; }

  template<typename Event>
  static auto project(const Event& evt);
};

struct ServerNetworkRouter
{
  struct state_type
  {};

  static auto project(const ServerAggregate::Spoke& evt)
  {
    return [evt](auto& network_client) {
      network_client.publish(evt);
      return 0;
    };
  }
};

struct ClientNetworkRouter
{
  struct state_type
  {};

  static auto project(const ClientAggregate::Spoke& evt)
  {
    return [evt](auto& network_client) {
      network_client.publish(evt);
      return 0;
    };
  }
};

template<>
auto
ServerAggregate::project(const ClientAggregate::Spoke& evt)
{
  return [evt](auto&) {
    std::cout << "Server: client said " << evt.sentence << std::endl;
    return 0;
  };
}

template<>
auto
ClientAggregate::project(const ServerAggregate::Spoke& evt)
{
  return [evt](auto&) {
    std::cout << "Client: server said " << evt.sentence << std::endl;
    return 0;
  };
}

namespace boost {
namespace serialization {

template<class Archive>
void
serialize(Archive& ar, ServerAggregate::Spoke& evt, const unsigned int version)
{
  ar& evt.sentence;
}

template<class Archive>
void
serialize(Archive& ar, ClientAggregate::Spoke& evt, const unsigned int version)
{
  ar& evt.sentence;
}

} // namespace serialization
} // namespace boost

int
main()
{
  static constexpr auto endpoint = [](const std::string& ip) { return "tcp://" + ip + ":5555"; };
  using event_type = std::variant<ServerAggregate::Spoke, ClientAggregate::Spoke>;

  using router_type = Router<event_type>;
  auto router = std::async(std::launch::async, router_type::event_loop, endpoint("*"));

  auto server_network = Client<event_type>{ endpoint("localhost") };
  auto server_ctx =
    event_sauce::context<decltype(server_network), ServerNetworkRouter, ServerAggregate>(server_network);
  server_network.start([&server_ctx](std::string from, const event_type& msg) { server_ctx.publish(msg); });

  auto client_network = Client<event_type>{ endpoint("localhost") };
  auto client_ctx =
    event_sauce::context<decltype(client_network), ClientNetworkRouter, ClientAggregate>(client_network);
  client_network.start([&client_ctx](std::string from, const event_type& msg) { client_ctx.publish(msg); });

  auto counter = 0;
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });
    server_ctx.dispatch(ServerAggregate::Speak{ "Hello " + std::to_string(counter) });
    client_ctx.dispatch(ClientAggregate::Speak{ "World " + std::to_string(counter) });
    ++counter;
  }

  return 0;
}
