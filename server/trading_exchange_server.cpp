#include "trading_exchange_server.h"
#include "global_trading_exchange_client.h"

TradingExchangeServer::TradingExchangeServer(
    boost::asio::io_service &io_service)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
      timer_(io_service, boost::asio::chrono::seconds(1)) {
  std::cout << "TradingExchangeServer started! Listen " << port << " port"
            << std::endl;

  Session *new_session = new Session(io_service_);
  acceptor_.async_accept(new_session->getSocket(),
                         boost::bind(&TradingExchangeServer::handleAccept, this,
                                     new_session,
                                     boost::asio::placeholders::error));

  startTimer();
}

void TradingExchangeServer::handleAccept(
    Session *new_session, const boost::system::error_code &error) {
  if (!error) {
    new_session->startSession();
    new_session = new Session(io_service_);
    acceptor_.async_accept(new_session->getSocket(),
                           boost::bind(&TradingExchangeServer::handleAccept,
                                       this, new_session,
                                       boost::asio::placeholders::error));
  } else {
    delete new_session;
  }
}

void TradingExchangeServer::startTimer() {
  timer_.async_wait(
      boost::bind(&TradingExchangeServer::tradingExchangeProcess, this));
}

void TradingExchangeServer::tradingExchangeProcess() {
  GlobalTradingExchangeClient().process();

  timer_.expires_at(timer_.expiry() + boost::asio::chrono::seconds(1));
  startTimer();
}