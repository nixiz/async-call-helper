#pragma once 
#include <memory>
#include "async-call-helper.hpp"

class safe_service 
  : public async_call_helper<safe_service>
{
public:
  explicit safe_service(int in_param);
  ~safe_service();

  void response(int out_param);
  void execute();
private:
  std::unique_ptr<int> param;
};