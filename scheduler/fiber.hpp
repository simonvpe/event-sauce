#pragma once
#include <boost/fiber/all.hpp>

class fiber_scheduler
{
public:
  using task_type = std::function<void()>;
  using channel_type = boost::fibers::buffered_channel<task_type>;

  fiber_scheduler(std::size_t serial_size, std::size_t concurrent_size)
      : serial_channel{ serial_size }
      , concurrent_channel{ concurrent_size }
  {}

  auto serial()
  {
    return [this](auto&& fn) { serial_channel.push(std::forward<decltype(fn)>(fn)); };
  }

  channel_type serial_channel;
  channel_type concurrent_channel;
};

class fiber_worker
{
public:
  fiber_worker(fiber_scheduler& scheduler, int concurrent_workers)
      : scheduler{ scheduler }
      , concurrent_workers{ concurrent_workers }
  {}

  auto run()
  {
    auto serial = boost::fibers::fiber([& channel = scheduler.serial_channel] {
      fiber_scheduler::task_type task;
      while (boost::fibers::channel_op_status::closed != channel.pop(task)) {
        task();
      }
    });

    auto concurrent = boost::fibers::fiber([& channel = scheduler.concurrent_channel] {
      fiber_scheduler::task_type task;
      while (boost::fibers::channel_op_status::closed != channel.pop(task)) {
        task();
      }
    });

    serial.join();
    concurrent.join();
  }

private:
  fiber_scheduler& scheduler;
  int concurrent_workers;
};
