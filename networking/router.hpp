#pragma once
#include "common.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <immer/set.hpp>
#include <iostream>
#include <sstream>

template <typename Message> struct Router {

public:
  using identity_type = std::string;
  using message_type = Message;

private:
  using broadcast_event_type = message_type;

  struct targetted_event_type {
    identity_type other;
    message_type message;

    template <typename Archive>
    void serialize(Archive &ar, const unsigned int version) {
      ar & this->other;
      ar & this->message;
    }
  };

  using event_type = std::variant<broadcast_event_type, targetted_event_type>;

  static std::string encode(const event_type &evt) {
    std::ostringstream ss;
    boost::archive::binary_oarchive oa{ss};
    oa << evt;
    return ss.str();
  }

  static event_type decode(const std::string &str) {
    event_type evt;
    std::istringstream ss{str};
    boost::archive::binary_iarchive ia{ss};
    ia >> evt;
    return std::move(evt);
  }

public:
  static void publish(zmq::socket_t &broker, const message_type &message) {
    send_more(broker, "");
    send_one(broker, encode(broadcast_event_type(message)));
  }

  static void publish(zmq::socket_t &broker, const identity_type &other,
                      const message_type &message) {
    send_more(broker, "");
    send_one(broker, encode(targetted_event_type{other, message}));
  }

  static message_type receive(zmq::socket_t &broker) {
    recv_one(broker, 0);
    auto body_parts = recv_one(broker, max_body_parts_size);
    const auto evt = decode(body_parts);

    if (std::holds_alternative<targetted_event_type>(evt)) {
      return std::get<targetted_event_type>(evt).message;
    }

    return std::get<broadcast_event_type>(evt);
  }

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
      return std::make_tuple(std::move(identity), decode(body_parts));
    };

    /***************************************************************************
     ** send
     ***************************************************************************/
    auto send = [&broker](const std::string &identity, const event_type &evt) {
      send_more(broker, identity);
      send_more(broker, "");
      send_one(broker, encode(evt));
    };

    /***************************************************************************
     ** loop
     ***************************************************************************/
    immer::set<std::string> clients;
    while (true) {
      auto [identity, evt] = recv();
      clients = clients.insert(identity);

      const auto others = clients.erase(identity);

      if (std::holds_alternative<targetted_event_type>(evt)) {
        auto targetted_event = std::get<targetted_event_type>(evt);
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
