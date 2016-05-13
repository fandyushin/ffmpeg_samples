#include <iostream>

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
}

int main(int argc, char const *argv[]) {

  av_register_all();
  std::cout << "Success!" << std::endl;

  exit(EXIT_SUCCESS);

  return 0;
}
