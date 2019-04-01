#pragma once
#include "../vendor/serialize_std_variant.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <iostream>
#include <optional>
#include <sstream>
#include <zmq.hpp>

static constexpr auto max_identity_size = 10;
static constexpr auto max_body_parts_size = 4096;

void
send_more(zmq::socket_t& broker, const std::string& message)
{
  zmq::message_t msg{ message.size() };
  memcpy(msg.data(), message.data(), message.size());
  broker.send(msg, ZMQ_SNDMORE);
};

void
send_one(zmq::socket_t& broker, const std::string& message)
{
  zmq::message_t msg{ message.size() };
  memcpy(msg.data(), message.data(), message.size());
  broker.send(msg);
};

auto
recv_one(zmq::socket_t& broker, std::size_t max_msg_size) -> std::string
{
  zmq::message_t msg{ max_msg_size };
  broker.recv(&msg);
  const auto* buffer = static_cast<const char*>(msg.data());
  return { buffer, buffer + msg.size() };
};

auto
recv_one_noblock(zmq::socket_t& broker, std::size_t max_msg_size) -> std::optional<std::string>
{
  zmq::message_t msg{ max_msg_size };
  if (broker.recv(&msg, ZMQ_NOBLOCK)) {
    const auto* buffer = static_cast<const char*>(msg.data());
    return { { buffer, buffer + msg.size() } };
  }
  return {};
}

template<typename Message>
struct event
{
  using identity_type = std::string;
  using message_type = Message;
  using broadcast_type = message_type;
  struct targetted_type
  {
    identity_type other;
    message_type message;

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
      ar & this->other;
      ar & this->message;
    }
  };
  using send_type = std::variant<broadcast_type, targetted_type>;

  struct recv_type
  {
    identity_type from;
    message_type message;

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
      ar & this->from;
      ar & this->message;
    }
  };
};
