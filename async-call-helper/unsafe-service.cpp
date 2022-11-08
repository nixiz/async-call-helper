#include <iostream>
#include "unsafe-service.h"
#include "external_c_lib.h"


unsafe_service::unsafe_service(int in_param)
  : param(new int(in_param))
{
  std::cout << "ctor  service \n";
}
unsafe_service::~unsafe_service() {
  std::cout << "~dtor service \n";
}

static inline void response_cb(void* context, int out_param) {
  auto srv_ptr = static_cast<unsafe_service*>(context);
  srv_ptr->response(out_param);
}

void unsafe_service::execute() {
  c_long_async_function((void*)this, response_cb, *param);
}

void unsafe_service::response(int out_param)
{
  std::cout << "received response for " << *param << ": " << out_param << "\n";
}