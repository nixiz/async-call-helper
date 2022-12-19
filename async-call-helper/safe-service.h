#pragma once 
#include <memory>
#include "async-call-helper.hpp"

enum class cb_type_e {
  c_style,
  cpp_lambda,
  cpp_bind
};

class safe_service 
  : public async_call_helper<safe_service, multi_thread_usage>
{
public:
  explicit safe_service(int in_param);
  ~safe_service();

  void response(int out_param);
  void execute();
private:
  void execute_c_style();
  void execute_cpp_lambda();
  void execute_cpp_bind();
  std::unique_ptr<int> param;
};