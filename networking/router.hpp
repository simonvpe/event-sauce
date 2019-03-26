#pragma once
#include "common.hpp"
#include <immer/set.hpp>

template <typename Message> struct Router {
public:
  using event_type = event<Message>;

  static auto event_loop(const std::string &endpoint) {
    /***************************************************************************
     ** setup
     ***************************************************************************/
    zmq::context_t zmq{1};
    zmq::socket_t broker{zmq, ZMQ_ROUTER};
    broker.bind(endpoint);

    /***************************************************************************
     ** recv
     ***************************************************************************/
    auto recv = [&broker] {
      auto identity = recv_one(broker, max_identity_size);
      recv_one(broker, 0); // delimiter
      auto body_parts = recv_one(broker, max_body_parts_size);
      auto evt = event_type::decode(body_parts);
      return std::make_tuple(std::move(identity), std::move(evt));
    };

    /***************************************************************************
     ** send
     ***************************************************************************/
    auto send = [&broker](const std::string &identity,
                          const typename event_type::type &evt) {
      send_more(broker, identity);
      send_more(broker, "");
      send_one(broker, event_type::encode(evt));
    };

    /***************************************************************************
     ** loop
     ***************************************************************************/
    immer::set<std::string> clients;

    while (true) {
      auto [identity, evt] = recv();

      clients = clients.insert(identity);

      const auto others = clients.erase(identity);

      if (std::holds_alternative<typename event_type::targetted_type>(evt)) {
        auto targetted_event =
            std::get<typename event_type::targetted_type>(evt);
        const auto destination = targetted_event.other;
        send(destination, targetted_event.message);
      } else {
        std::for_each(
            others.begin(), others.end(),
            [&send, &evt](const auto &identity) { send(identity, evt); });
      }
    }
  }
};
