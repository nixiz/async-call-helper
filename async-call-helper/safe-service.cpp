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
  auto srv_ptr = asyn_call_token::from_context<safe_service>(context);
  if (srv_ptr) {
    srv_ptr->response(out_param);
  } else {
    std::cerr << "no service instance to make callback call\n";
  }
}

void safe_service::execute() {
  // c::long_async_function(get_context(), safe_response_cb, *param);

  // auto context = get_context<int>([this](int o) {
  //   std::cout << "received response from lambda " << *param << ": " << o << "\n";
  // });
  // c::long_async_function(context.context, context.callback, *param);

  using namespace std::placeholders;
  auto context_bind = get_context<int>(std::bind(&safe_service::response, this, _1));
  c_long_async_function(context_bind.context, context_bind.callback, *param * 3);

  //cpp::long_async_function(context_bind, *param);
}

void safe_service::response(int out_param) {
  std::cout << "received response. this->param: " << *param << "\n";
}