#include "aggregates/player.hpp"
#include "aggregates/time.hpp"
#include "networking/router.hpp"
#include "vendor/event-sauce.hpp"
#include <future>
#include <immer/map.hpp>
#include <iostream>

struct ReadModel {};

struct ConnectionPool {
public:
  static constexpr auto project = [] {}; // Disabled
  static constexpr auto process = [] {}; // Disabled
  static constexpr auto apply = [] {};
  static constexpr auto execute = [] {};

  using ClientIdentity = std::string;

  //////////////////////////////////////////////////////////////////////////////
  // Commands
  //////////////////////////////////////////////////////////////////////////////
  struct StartListening {
    CorrelationId correlation_id;
  };

  struct connection_t {
    CorrelationId correlation_id;
  };

  struct state_type {
    immer::map<ClientIdentity, connection_t> clients;
  };
};

int main() {
  static constexpr auto endpoint = [](const std::string &ip) {
    return "tcp://" + ip + ":5555";
  };

  auto read_model = ReadModel{};
  auto ctx = event_sauce::make_context<Player, Entity, Time, RigidBody,
                                       ConnectionPool>(read_model);

  auto server = std::async(std::launch::async, Router<std::string>::router,
                           endpoint("*"));

  auto client = [](const std::string &name) {
    zmq::context_t zmq{1};
    zmq::socket_t broker{zmq, ZMQ_DEALER};
    broker.connect(endpoint("localhost"));

    while (true) {
      Router<std::string>::publish(broker, "Hello from " + name);
      const auto evt = Router<std::string>::receive(broker);
      std::cout << name << " received " << evt << std::endl;
    }
  };

  auto c1 = std::async(std::launch::async, client, "A");
  auto c2 = std::async(std::launch::async, client, "B");

  return 0;
}
