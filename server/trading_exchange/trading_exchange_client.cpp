#include "trading_exchange_client.h"

void TradingExchangeClient::process() { matchOrders(); }

void TradingExchangeClient::matchOrders() {
  if (orderBookToBuy_.empty() || orderBookToSell_.empty()) {
    return;
  }

  common::Order orderToSell = *orderBookToSell_.begin();
  common::Order orderToBuy = *orderBookToBuy_.begin();
  common::CurrencyTypeValue price(orderToSell.price.first, 0.f);
  common::CurrencyTypeValue volume(orderToSell.volume.first, 0.f);
  if ((orderToSell.volume.first != orderToBuy.volume.first) ||
      (orderToSell.price.first != orderToBuy.price.first)) {
    return;
  }

  if (orderToBuy.price.second >= orderToSell.price.second) {
    cancelOrder(orderToSell);
    cancelOrder(orderToBuy);
    if (orderToSell.volume.second > orderToBuy.volume.second) {
      orderToSell.volume.second -= orderToBuy.volume.second;
      price.second = orderToBuy.price.second * orderToBuy.volume.second;
      volume.second = orderToBuy.volume.second;
      registerOrder(orderToSell);
    } else if (orderToSell.volume.second < orderToBuy.volume.second) {
      orderToBuy.volume.second -= orderToSell.volume.second;
      volume.second = orderToSell.volume.second;
      price.second = orderToBuy.price.second * orderToSell.volume.second;
      registerOrder(orderToBuy);
    }
    withdraw(orderToSell.userID, volume);
    deposit(orderToBuy.userID, volume);

    std::cout << orderToBuy.price.second << " " << price.second << std::endl;
    deposit(orderToSell.userID, price);
    withdraw(orderToBuy.userID, price);
  }
}

std::string
TradingExchangeClient::registerNewUser(const std::string &userName) {
  for (const auto &userPair : users) {
    if (userPair.second.name == userName) {
      return userPair.second.userId;
    }
  }
  size_t newUserId = users.size();
  users[newUserId].name = userName;
  users[newUserId].userId = std::to_string(newUserId);
  for (const std::string currency : common::currency::currencyTypes) {
    users[newUserId].balance[currency] = 0.f;
  }

  return std::to_string(newUserId);
}

TradingExchangeClient::User
TradingExchangeClient::getUserById(const std::string &userId) {
  const auto userIt = users.find(std::stoi(userId));
  if (userIt == users.cend()) {
    std::cout << "Error! Unknown User" << std::endl;
    User unknownUser;
    return unknownUser;
  } else {
    return userIt->second;
  }
}

TradingExchangeClient::User
TradingExchangeClient::getUserByName(const std::string &userName) {
  for (const auto &userPair : users) {
    if (userPair.second.name == userName) {
      return userPair.second;
    }
  }
  return User();
}

std::string TradingExchangeClient::registerOrder(const common::Order &order) {
  User user = getUserById(order.userID);
  if (user.name != "Unknown User") {
    order.type == common::OrderType_Buy ? orderBookToBuy_.insert(order)
                                        : orderBookToSell_.insert(order);
    users.find(std::stoi(order.userID))->second.orders.insert(order);
    return "Order registration accepted";
  }
  return user.name;
}

std::string TradingExchangeClient::cancelOrder(const common::Order &order) {
  User user = getUserById(order.userID);
  if (user.name != "Unknown User") {
    order.type == common::OrderType_Buy ? orderBookToBuy_.erase(order)
                                        : orderBookToSell_.erase(order);
    users.find(std::stoi(order.userID))->second.orders.erase(order);
    return "Order cancel accepted";
  }
  return user.name;
}

std::string TradingExchangeClient::withdraw(
    const std::string &userId,
    const common::CurrencyTypeValue &currencyTypeValue) {
  User user = getUserById(userId);
  if (user.name != "Unknown User") {
    users.find(std::stoi(userId))->second.balance[currencyTypeValue.first] -=
        currencyTypeValue.second;
    return "Withdraw accepted";
  }
  return user.name;
}

std::string TradingExchangeClient::deposit(
    const std::string &userId,
    const common::CurrencyTypeValue &currencyTypeValue) {
  User user = getUserById(userId);
  if (user.name != "Unknown User") {
    users.find(std::stoi(userId))->second.balance[currencyTypeValue.first] +=
        currencyTypeValue.second;
    return "Deposit accepted";
  }
  return user.name;
}
