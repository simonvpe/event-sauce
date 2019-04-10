#include "pool_dispatcher.hpp"

pool_dispatcher::pool_dispatcher(int nthreads)
    : io_context{}
    , work{ std::make_unique<boost::asio::io_context::work>(io_context) }
    , pool{}
    , serializer{ io_context }
    , signal_set{ io_context }
{
  for (auto i = 0; i < nthreads; ++i) {
    pool.create_thread([this] { io_context.run(); });
  }
}

pool_dispatcher::~pool_dispatcher()
{
  work.reset();
  pool.join_all();
}

void
pool_dispatcher::install_signal_handler(const std::vector<int>& signals, std::function<void()> callback)
{
  std::for_each(signals.begin(), signals.end(), [&](int sig) { signal_set.add(sig); });
  signal_set.async_wait([=](boost::system::error_code ec, int) {
    if (!ec) {
      callback();
    } else {
      throw std::runtime_error{ ec.message() };
    }
  });
}
