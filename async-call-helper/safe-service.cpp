#include "safe-service.h"
#include <iostream>
#include "external_c_lib.h"

safe_service::safe_service(int in_param) : param(new int(in_param)) {
  std::cout << "ctor  service \n";
}

safe_service::~safe_service() {
  set_deleted();
  std::cout << "~dtor service \n";
}

static inline void safe_response_cb(void* context, int out_param) {
  std::cout << "safe_response_cb\n";
  auto srv_ptr = asyn_call_token::from_context<safe_service>(context);
  if (srv_ptr) {
    srv_ptr->response(out_param);
  } else {
    std::cerr << "no service instance to make callback call\n";
  }
}

void safe_service::execute_c_style() 
{
  c_long_async_function(get_context(), safe_response_cb, *param);
}

void safe_service::execute_cpp_lambda() 
{
  auto context = get_context<int>([this](int o) {
    std::cout << "received response from lambda " << *param << ": " << o << "\n";
  });
  c_long_async_function(context.context, context.callback, *param);
}

void safe_service::execute_cpp_bind() 
{
  using namespace std::placeholders;
  auto context_bind = get_context<int>(std::bind(&safe_service::response, this, _1));
  c_long_async_function(context_bind.context, context_bind.callback, *param * 3);
}

void safe_service::response(int out_param) {
  std::cout << "response\n";
  std::cout << "received response. this->param: " << *param << "\n";
}

void safe_service::execute() 
{
  constexpr const auto cb_type = cb_type_e::c_style;
  switch (cb_type)
  {
  case cb_type_e::c_style:      execute_c_style();    break;
  case cb_type_e::cpp_bind:     execute_cpp_bind();   break;
  case cb_type_e::cpp_lambda:   execute_cpp_lambda(); break;
  }
}
