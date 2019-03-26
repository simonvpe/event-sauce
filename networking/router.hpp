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
                          const typename event_type::recv_type &evt) {
      send_more(broker, identity);
      send_more(broker, "");
      send_one(broker, event_type::encode(evt));
    };

    /***************************************************************************
     ** loop
     ***************************************************************************/
    immer::set<std::string> clients;

    while (true) {
      auto [source, evt] = recv();

      clients = clients.insert(source);

      const auto others = clients.erase(source);

      if (std::holds_alternative<typename event_type::targetted_type>(evt)) {
        auto targetted_event =
            std::get<typename event_type::targetted_type>(evt);
        const auto destination = targetted_event.other;
        const auto msg =
            typename event_type::recv_type{source, targetted_event.message};
        send(destination, msg);
      } else if (std::holds_alternative<typename event_type::broadcast_type>(
                     evt)) {
        auto broadcast_event =
            std::get<typename event_type::broadcast_type>(evt);
        std::for_each(
            others.begin(), others.end(), [&](const auto &destination) {
              const auto msg =
                  typename event_type::recv_type{source, broadcast_event};
              send(destination, msg);
            });
      }
    }
  }
};
