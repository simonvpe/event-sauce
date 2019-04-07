#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/post.hpp>
#include <boost/thread/thread.hpp>
#include <thread>

class pool_dispatcher
{
  boost::asio::io_context io_context;
  std::unique_ptr<boost::asio::io_context::work> work;
  boost::thread_group pool;
  boost::asio::io_context::strand serializer;

public:
  pool_dispatcher(int nthreads = std::thread::hardware_concurrency())
      : io_context{}
      , work{ std::make_unique<boost::asio::io_context::work>(io_context) }
      , pool{}
      , serializer{ io_context }
  {
    for (auto i = 0; i < nthreads; ++i) {
      pool.create_thread([this] { io_context.run(); });
    }
  }

  ~pool_dispatcher()
  {
    work.reset();
    pool.join_all();
  }

  auto serial()
  {
    return [this](auto&& fn) { boost::asio::post(serializer, std::forward<decltype(fn)>(fn)); };
  }

  auto concurrent()
  {
    return [this](auto&& fn) { boost::asio::post(io_context, std::forward<decltype(fn)>(fn)); };
  }
};
