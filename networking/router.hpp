#pragma once
#include "common.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <immer/set.hpp>
#include <iostream>
#include <sstream>

using Identity = std::string;
using Source = Identity;
using Message = std::string;

using BroadcastEvent = Message;

struct TargettedEvent {
  Identity other;
  Message message;
};

using Event = std::variant<BroadcastEvent, TargettedEvent>;

template <typename Archive>
void serialize(Archive &ar, TargettedEvent &evt, const unsigned int version) {
  ar &evt.other;
  ar &evt.message;
}

std::string encode(const Event &evt) {
  std::ostringstream ss;
  boost::archive::binary_oarchive oa{ss};
  oa << evt;
  return ss.str();
}

Event decode(const std::string &str) {
  Event evt;
  std::istringstream ss{str};
  boost::archive::binary_iarchive ia{ss};
  ia >> evt;
  return std::move(evt);
}

void publish(zmq::socket_t &broker, const Message &message) {
  send_more(broker, "");
  send_one(broker, encode(BroadcastEvent(message)));
}

void publish(zmq::socket_t &broker, const Identity &other,
             const Message &message) {
  send_more(broker, "");
  send_one(broker, encode(TargettedEvent{other, message}));
}

Message receive(zmq::socket_t &broker) {
  recv_one(broker, 0);
  auto body_parts = recv_one(broker, max_body_parts_size);
  const auto evt = decode(body_parts);

  if (std::holds_alternative<TargettedEvent>(evt)) {
    return std::get<TargettedEvent>(evt).message;
  }

  return std::get<BroadcastEvent>(evt);
}

auto router(const std::string &endpoint) {
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
  auto send = [&broker](const std::string &identity, const Event &evt) {
    send_more(broker, identity);
    send_more(broker, "");
    send_one(broker, encode(evt));
  };

  immer::set<std::string> clients;

  while (true) {
    auto [identity, evt] = recv();
    clients = clients.insert(identity);

    const auto others = clients.erase(identity);

    if (std::holds_alternative<TargettedEvent>(evt)) {
      auto targetted_event = std::get<TargettedEvent>(evt);
      const auto destination = targetted_event.other;
      send(destination, targetted_event.message);
    } else {
      std::for_each(
          others.begin(), others.end(),
          [&send, &evt](const auto &identity) { send(identity, evt); });
    }
  }
}
