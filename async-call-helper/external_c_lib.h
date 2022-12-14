#pragma once

#include <chrono>
#include <functional>
#include <thread>

namespace {
typedef void (*long_async_function_cb)(void*, int);

static void c_long_async_function(void* context,
                                  long_async_function_cb cb,
                                  int in_param,
                                  int sleep_msec = 1001) 
{
  std::thread([=] {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_msec));
    auto out_param = in_param * 2;
    cb(context, out_param);
  }).detach();
}

}  // namespace c

namespace cpp {
using long_async_function_cb_t = std::function<void(int)>;

static void long_async_function(long_async_function_cb_t cb, int in_param, int sleep_msec = 1001) {
  std::thread([=] {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_msec));
    auto out_param = in_param * 2;
    cb(out_param);
  }).detach();
}

}  // namespace cpp