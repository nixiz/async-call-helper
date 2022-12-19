#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include "unsafe-service.h"
#include "safe-service.h"

using namespace std::chrono_literals;

int main()
{
#if 1
  {
   unsafe_service s(5);
   s.execute();
   std::this_thread::sleep_for(1s);
  }
  (void) getchar();
#else
  do
  {
    safe_service s(5);
    s.execute();
    std::this_thread::sleep_for(3s);
  } while(getchar() != 'q');
#endif
	return 0;
}