
#include "pa_utility.h"
#include "Phone.h"
#include "socks.h"

#include <iostream>
#include <cstring>
#include <string>
#include <cmath>
#include <deque>
#include <mutex>
#include <thread>
#include <cerrno>


int main(int argc, char *argv[]) {  

  Phone p(argc, argv);
  p.run();

  return 0;
}
