#include "session.h"
#include "global_trading_exchange_client.h"

Session::Session(boost::asio::io_service &io_service) : socket_(io_service) {}

tcp::socket &Session::getSocket() { return socket_; }

void Session::startSession() {
  socket_.async_read_some(
      boost::asio::buffer(data_, limits::buffSize),
      boost::bind(&Session::handleRead, this, boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void Session::handleRead(const boost::system::error_code &error,
                         size_t bytes_transferred) {
  if (!error) {
    data_[bytes_transferred] = '\0';
    const nlohmann::json j = nlohmann::json::parse(data_);
    const std::string response = createResponse(j);
    boost::asio::async_write(socket_,
                             boost::asio::buffer(response, response.size()),
                             boost::bind(&Session::handleWrite, this,
                                         boost::asio::placeholders::error));
  } else {
    delete this;
  }
}

void Session::handleWrite(const boost::system::error_code &error) {
  if (!error) {
    socket_.async_read_some(
        boost::asio::buffer(data_, limits::buffSize),
        boost::bind(&Session::handleRead, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  } else {
    delete this;
  }
}

common::Order
Session::createOrderFromRequest(const std::string &request) const {
  std::cout << request << std::endl;
  common::Order order;
  nlohmann::json j = nlohmann::json::parse(request);
  order.userID = j["userID"].get<std::string>();
  order.volume.first = j["volume"]["currencyType"].get<std::string>();
  order.volume.second = j["volume"]["value"].get<float>();
  order.price.first = j["price"]["currencyType"].get<std::string>();
  order.price.second = j["price"]["value"].get<float>();
  order.time = j["time"].get<std::time_t>();
  order.type = j["type"].get<common::OrderType>();
  return order;
}

std::string Session::createResponse(const nlohmann::json &json) {
  const std::string reqType = json["ReqType"];
  const std::string reqMessage = json["Message"];
  const std::string reqUserId = json["UserId"];

  if (reqType == common::requests::SignIn) {
    return GlobalTradingExchangeClient().getUserByName(reqMessage).userId;
  } else if (reqType == common::requests::SignUp) {
    return GlobalTradingExchangeClient().registerNewUser(reqMessage);
  } else if ((reqType == common::requests::Buy) ||
             (reqType == common::requests::Sell)) {
    const common::Order order = createOrderFromRequest(reqMessage);
    return GlobalTradingExchangeClient().registerOrder(order);
  } else if (reqType == common::requests::Balance) {
    nlohmann::json j;
    const TradingExchangeClient::User user =
        GlobalTradingExchangeClient().getUserById(reqUserId);
    for (const auto &currencyTypeValue : user.balance) {
      j[currencyTypeValue.first] = currencyTypeValue.second;
    }
    return j.dump();
  } else if (reqType == common::requests::Deposit) {
    nlohmann::json j = nlohmann::json::parse(reqMessage);
    const common::CurrencyTypeValue pair(
        j["pair"].get<common::CurrencyTypeValue>());
    return GlobalTradingExchangeClient().deposit(reqUserId, pair);
  } else if (reqType == common::requests::Withdraw) {
    nlohmann::json j = nlohmann::json::parse(reqMessage);
    const common::CurrencyTypeValue pair(
        j["pair"].get<common::CurrencyTypeValue>());
    return GlobalTradingExchangeClient().withdraw(reqUserId, pair);
  } else if (reqType == common::requests::Orders) {
    const TradingExchangeClient::User user =
        GlobalTradingExchangeClient().getUserById(reqUserId);
    if (user.orders.empty()) {
      return "No orders";
    }
    nlohmann::json j;
    std::size_t num = 0;
    for (const auto &order : user.orders) {
      ++num;
      j[std::to_string(num)] = {{"volume",
                                 {{"currencyType", order.volume.first},
                                  {"value", order.volume.second}}},
                                {"price",
                                 {{"currencyType", order.price.first},
                                  {"value", order.price.second}}},
                                {"type", order.type},
                                {"userID", order.userID},
                                {"time", order.time}};
    }
    j["count"] = num;
    return j.dump();
  } else if (reqType == common::requests::Cancel) {
    std::cout << reqMessage << std::endl;
    const common::Order order = createOrderFromRequest(reqMessage);
    return GlobalTradingExchangeClient().cancelOrder(order);
  }
  return "Unknown request";
}
