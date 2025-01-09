#include "user_client.h"

int main() {
  boost::asio::io_service io_service;
  UserClient client("127.0.0.1", 5555, io_service);
  client.process();

  return 0;
}