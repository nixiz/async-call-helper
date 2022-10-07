#pragma once 
#include <memory>

class unsafe_service
{
public:
  explicit unsafe_service(int in_param);
  ~unsafe_service();

  void response(int out_param);
  void execute();
private:
  std::unique_ptr<int> param;
};