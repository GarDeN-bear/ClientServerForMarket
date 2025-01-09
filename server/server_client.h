#include "Common.hpp"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <queue>
#include <set>

class server {
public:
  server(boost::asio::io_service &io_service)
      : io_service_(io_service),
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
        timer_(io_service, boost::asio::chrono::seconds(1)) {
    std::cout << "Server started! Listen " << port << " port" << std::endl;

    session *new_session = new session(io_service_);
    acceptor_.async_accept(new_session->socket(),
                           boost::bind(&server::handle_accept, this,
                                       new_session,
                                       boost::asio::placeholders::error));

    // Запуск таймера
    start_timer();
  }

  void handle_accept(session *new_session,
                     const boost::system::error_code &error) {
    if (!error) {
      new_session->start();
      new_session = new session(io_service_);
      acceptor_.async_accept(new_session->socket(),
                             boost::bind(&server::handle_accept, this,
                                         new_session,
                                         boost::asio::placeholders::error));
    } else {
      delete new_session;
    }
  }

private:
  boost::asio::io_service &io_service_;
  tcp::acceptor acceptor_;
  boost::asio::steady_timer timer_;

  void start_timer() {
    timer_.async_wait(boost::bind(&server::process_orders, this));
  }

  void process_orders() {
    // Вызов метода process
    GetCore().process();

    // Перезапуск таймера
    timer_.expires_at(timer_.expiry() + boost::asio::chrono::seconds(1));
    start_timer();
  }
};
