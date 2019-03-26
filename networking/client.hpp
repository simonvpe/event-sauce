#pragma once
#include "common.hpp"
#include <deque>
#include <future>

template <typename Message> struct Client {
public:
  using event_type = event<Message>;

  using callback_type = std::function<void(typename event_type::identity_type,
                                           typename event_type::message_type)>;

  class context_type {
  private:
    zmq::context_t zmq;
    zmq::socket_t broker;
    std::deque<std::function<void()>> event_queue;
    std::future<void> event_loop_instance;
    std::mutex key;

    static void dispatch(callback_type &cb,
                         const typename event_type::recv_type &msg) {
      cb(msg.from, msg.message);
    }

    static typename event_type::recv_type decode(const std::string &str) {
      typename event_type::recv_type evt;
      std::istringstream ss{str};
      boost::archive::binary_iarchive ia{ss};
      ia >> evt;
      return std::move(evt);
    }

    void event_loop(callback_type cb) {
      while (true) {
        // Recv
        while (const auto msg = recv_one_noblock(broker, max_body_parts_size)) {
          if (msg->size() > 0) {
            auto m = context_type::decode(*msg);
            cb(m.from, m.message);
          }
        }

        // Send
        std::lock_guard<std::mutex> guard{key};
        while (!event_queue.empty()) {
          event_queue.front()();
          event_queue.pop_front();
        }
      }
    }

  public:
    context_type(const std::string &endpoint, callback_type callback)
        : zmq{1}, broker{zmq, ZMQ_DEALER} {
      broker.connect(endpoint);
      event_loop_instance =
          std::async(std::launch::async, &context_type::event_loop, this,
                     std::move(callback));
    }

    void publish(const typename event_type::message_type &message) {
      auto msg =
          event_type::encode(typename event_type::broadcast_type{message});
      std::lock_guard<std::mutex> guard{key};
      event_queue.push_back([msg = std::move(msg), this] {
        send_more(this->broker, "");
        send_one(this->broker, std::move(msg));
      });
    }

    void publish(const typename event_type::message_type &message,
                 const typename event_type::identity_type &identity) {
      auto msg = event_type::encode(
          typename event_type::targetted_type{identity, message});
      std::lock_guard<std::mutex> guard{key};
      event_queue.push_back([msg = std::move(msg), this] {
        send_more(this->broker, "");
        send_one(broker, std::move(msg));
      });
    }
  };

  static std::unique_ptr<context_type> start(const std::string &endpoint,
                                             callback_type callback) {
    return std::make_unique<context_type>(endpoint, std::move(callback));
  }
};
