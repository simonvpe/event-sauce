#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread/thread.hpp>
#include <functional>
#include <thread>
#include <vector>

class pool_dispatcher
{
  boost::asio::io_context io_context;
  std::unique_ptr<boost::asio::io_context::work> work;
  boost::thread_group pool;
  boost::asio::io_context::strand serializer;
  boost::asio::signal_set signal_set;

public:
  pool_dispatcher(int nthreads = std::thread::hardware_concurrency());

  ~pool_dispatcher();

  boost::asio::io_context& service()
  {
    return io_context;
  }

  auto serial()
  {
    return [this](auto&& fn) { boost::asio::post(serializer, std::forward<decltype(fn)>(fn)); };
  }

  auto concurrent()
  {
    return [this](auto&& fn) { boost::asio::post(io_context, std::forward<decltype(fn)>(fn)); };
  }

  void install_signal_handler(const std::vector<int>& signals, std::function<void()> callback);
};
