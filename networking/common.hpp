#pragma once
#include "../vendor/serialize_std_variant.hpp"
#include <zmq.hpp>

static constexpr auto max_identity_size = 10;
static constexpr auto max_body_parts_size = 4096;

void send_more(zmq::socket_t &broker, const std::string &message) {
  zmq::message_t msg{message.size()};
  memcpy(msg.data(), message.data(), message.size());
  broker.send(msg, ZMQ_SNDMORE);
};

void send_one(zmq::socket_t &broker, const std::string &message) {
  zmq::message_t msg{message.size()};
  memcpy(msg.data(), message.data(), message.size());
  broker.send(msg);
};

auto recv_one(zmq::socket_t &broker, std::size_t max_msg_size) -> std::string {
  zmq::message_t msg{max_msg_size};
  broker.recv(&msg);
  const auto *buffer = static_cast<const char *>(msg.data());
  return {buffer, buffer + msg.size()};
};
