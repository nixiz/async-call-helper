#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include "unsafe-service.h"
#include "safe-service.h"

using namespace std::chrono_literals;

int main()
{
  //std::cout << "will run unsafe service\n";
  //{
  //  unsafe_service s(5);
  //  s.execute();
  //  std::this_thread::sleep_for(1s);
  //}
  //(void) getchar();
  std::cout << "will run safe service with same scenario\n\n";
  do
  {
    safe_service s(5);
    s.execute();
    std::this_thread::sleep_for(3s);
  } while(getchar() != 'q');

	return 0;
}