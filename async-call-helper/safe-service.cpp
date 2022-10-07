#include <iostream>
#include "safe-service.h"
#include "external_c_lib.h"


safe_service::safe_service(int in_param)
  : param(new int(in_param))
{
  std::cout << "ctor  service \n";
}

safe_service::~safe_service() {
  std::cout << "~dtor service \n";
  set_deleted();
}

static inline void safe_response_cb(void* context, int out_param) {
  auto srv_ptr = asyn_call_token::from_context<safe_service>(context);
  if (srv_ptr)
  {
    srv_ptr->response(out_param);
  }
  else
  {
    std::cerr << "no service instance to make callback call\n";
  }
  
}

void safe_service::execute() {
  c::long_async_function(get_context(), safe_response_cb, *param);
}

void safe_service::response(int out_param)
{
  std::cout << "received response for " << *param << ": " << out_param << "\n";
}