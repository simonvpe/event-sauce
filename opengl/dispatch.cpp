#include "include/dispatch.hpp"

pool_dispatcher::pool_dispatcher(int nthreads)
    : io_context{}
    , work{ std::make_unique<boost::asio::io_context::work>(io_context) }
    , pool{}
    , serializer{ io_context }
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
