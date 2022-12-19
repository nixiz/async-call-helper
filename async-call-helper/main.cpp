#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include "unsafe-service.h"
#include "safe-service.h"

using namespace std::chrono_literals;

int main(int argc, const char*[])
{
  bool run_unsafe = argc == 1;
  if (run_unsafe)
  {
   unsafe_service s(5);
   s.execute();
   std::this_thread::sleep_for(1s);
  }
  else 
  {
    safe_service s(5);
    s.execute();
    std::this_thread::sleep_for(1s);
  }
  (void)getchar();
	return 0;
}