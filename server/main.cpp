#include "server_client.h"

int main() {
  try {
    boost::asio::io_service io_service;
    static Core core;

    server s(io_service);

    io_service.run();
  } catch (std::exception &e) {
    std::cerr << "Server exception: " << e.what() << "\n";
  }

  return 0;
}