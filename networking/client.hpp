#pragma once
#include "common.hpp"
#include <deque>
#include <future>

template<typename Message>
struct Client
{
public:
  using event_type = event<Message>;

  using callback_type = std::function<void(typename event_type::identity_type, typename event_type::message_type)>;

private:
  zmq::context_t zmq;
  zmq::socket_t broker;
  std::deque<std::function<void()>> event_queue;
  std::future<void> event_loop_instance;
  std::mutex key;

  static void dispatch(callback_type& cb, const typename event_type::recv_type& msg) { cb(msg.from, msg.message); }

  static typename event_type::recv_type decode(const std::string& str)
  {
    typename event_type::recv_type evt;
    std::istringstream ss{ str };
    boost::archive::binary_iarchive ia{ ss };
    ia >> evt;
    return std::move(evt);
  }

  static std::string encode(const typename event_type::send_type& evt)
  {
    std::ostringstream ss;
    boost::archive::binary_oarchive oa{ ss };
    oa << evt;
    return ss.str();
  }

  void notify_presence()
  {
    auto msg = encode(typename event_type::notify_presence{});
    std::lock_guard<std::mutex> guard{ key };
    event_queue.push_back([msg = std::move(msg), this] {
      send_more(this->broker, "");
      send_one(this->broker, std::move(msg));
    });
  }

  void event_loop(callback_type cb)
  {
    while (true) {
      // Recv
      while (const auto msg = recv_one_noblock(broker, max_body_parts_size)) {
        if (msg->size() > 0) {
          auto m = Client::decode(*msg);
          cb(m.from, m.message);
        }
      }

      // Send
      std::lock_guard<std::mutex> guard{ key };
      while (!event_queue.empty()) {
        event_queue.front()();
        event_queue.pop_front();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
  }

public:
  Client(const std::string& endpoint)
      : zmq{ 1 }
      , broker{ zmq, ZMQ_DEALER }
  {
    broker.connect(endpoint);
  }

  Client(const std::string& endpoint, callback_type&& callback)
      : Client{ endpoint }
  {
    start(std::forward<callback_type>(callback));
  }

  void start(callback_type&& cb)
  {
    event_loop_instance = std::async(std::launch::async, &Client::event_loop, this, std::forward<callback_type>(cb));
    notify_presence();
  }

  void publish(const typename event_type::message_type& message)
  {
    auto msg = encode(typename event_type::broadcast_type{ message });
    std::lock_guard<std::mutex> guard{ key };
    event_queue.push_back([msg = std::move(msg), this] {
      send_more(this->broker, "");
      send_one(this->broker, std::move(msg));
    });
  }

  void publish(const typename event_type::message_type& message, const typename event_type::identity_type& identity)
  {
    auto msg = encode(typename event_type::targetted_type{ identity, message });
    std::lock_guard<std::mutex> guard{ key };
    event_queue.push_back([msg = std::move(msg), this] {
      send_more(this->broker, "");
      send_one(this->broker, std::move(msg));
    });
  }
};
