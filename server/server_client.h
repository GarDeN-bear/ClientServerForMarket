#pragma once

#include "common.h"
#include "session.h"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <queue>
#include <set>

class Server {
public:
  Server(boost::asio::io_service &io_service);

  void handleAccept(Session *new_session,
                    const boost::system::error_code &error);

private:
  boost::asio::io_service &io_service_;
  tcp::acceptor acceptor_;
  boost::asio::steady_timer timer_;

  void startTimer();

  void processOrders();
};
