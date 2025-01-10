#include "server_client.h"
#include "global_trading_exchange_client.h"

Server::Server(boost::asio::io_service &io_service)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
      timer_(io_service, boost::asio::chrono::seconds(1)) {
  std::cout << "Server started! Listen " << port << " port" << std::endl;

  Session *new_session = new Session(io_service_);
  acceptor_.async_accept(new_session->getSocket(),
                         boost::bind(&Server::handleAccept, this, new_session,
                                     boost::asio::placeholders::error));

  startTimer();
}

void Server::handleAccept(Session *new_session,
                          const boost::system::error_code &error) {
  if (!error) {
    new_session->startSession();
    new_session = new Session(io_service_);
    acceptor_.async_accept(new_session->getSocket(),
                           boost::bind(&Server::handleAccept, this, new_session,
                                       boost::asio::placeholders::error));
  } else {
    delete new_session;
  }
}

void Server::startTimer() {
  timer_.async_wait(boost::bind(&Server::processOrders, this));
}

void Server::processOrders() {
  GlobalTradingExchangeClient().process();

  timer_.expires_at(timer_.expiry() + boost::asio::chrono::seconds(1));
  startTimer();
}