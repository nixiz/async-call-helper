#pragma once

#include <thread>
#include <chrono>

namespace c {
  typedef void(*long_async_function_cb)(void*, int);

  static void long_async_function(void* context, long_async_function_cb cb, int in_param, int sleep_msec = 1000) {
    std::thread([=] {
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_msec));
      auto out_param = in_param * 2;
      cb(context, out_param);
      }).detach();
  }
}