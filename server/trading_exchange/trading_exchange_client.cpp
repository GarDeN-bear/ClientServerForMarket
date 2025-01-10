#include "trading_exchange_client.h"

/**
 * @brief Обработка заявок в "стакане".
 */
void TradingExchangeClient::process() { matchOrders(); }

void TradingExchangeClient::matchOrders() {
  if (orderBookToBuy_.empty() || orderBookToSell_.empty()) {
    return;
  }

  common::Order orderToSell = *orderBookToSell_.begin();
  common::Order orderToBuy = *orderBookToBuy_.begin();
  common::CurrencyTypeValue price(orderToSell.price.first, 0.f);
  common::CurrencyTypeValue volume(orderToSell.volume.first, 0.f);

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
/**
 * @brief Зарегистрировать нового пользователя.
 * @param aUserName Имя пользователя.
 * @return ID нового пользователя.
 */
std::string
TradingExchangeClient::registerNewUser(const std::string &aUserName) {
  size_t newUserId = mUsers.size();
  mUsers[newUserId].name = aUserName;
  for (const std::string currency : common::currency::currencyTypes) {
    mUsers[newUserId].balance[currency] = 0.f;
  }
  return std::to_string(newUserId);
}

/**
 * @brief Получить информацию о клиенте.
 * @param aUserId ID пользователя.
 * @return Пользователь.
 */
TradingExchangeClient::User
TradingExchangeClient::getUser(const std::string &aUserId) {
  const auto userIt = mUsers.find(std::stoi(aUserId));
  if (userIt == mUsers.cend()) {
    std::cout << "Error! Unknown User" << std::endl;
    User unknownUser;
    return unknownUser;
  } else {
    return userIt->second;
  }
}

/**
 * @brief Зарегистрировать заявку на покупку/продажу.
 * @param order Заявка.
 * @return Результат регистрации заявки.
 */
std::string TradingExchangeClient::registerOrder(const common::Order &order) {
  User user = getUser(order.userID);
  if (user.name != "Unknown User") {
    order.type == common::OrderType_Buy ? orderBookToBuy_.insert(order)
                                        : orderBookToSell_.insert(order);
    mUsers.find(std::stoi(order.userID))->second.orders.insert(order);
    std::cout << mUsers.find(std::stoi(order.userID))->second.orders.size()
              << std::endl;
    return "-->Order to " +
           std::string(order.type == common::OrderType_Buy ? "buy " : "sell ") +
           std::to_string(order.volume.second) + order.volume.first + " for " +
           std::to_string(order.price.second) + order.price.first +
           " apiece accepted";
  }
  return user.name;
}

/**
 * @brief Отменить заявку на покупку/продажу.
 * @param order Заявка.
 * @return Результат отмены заявки.
 */
std::string TradingExchangeClient::cancelOrder(const common::Order &order) {
  User user = getUser(order.userID);
  if (user.name != "Unknown User") {
    order.type == common::OrderType_Buy ? orderBookToBuy_.erase(order)
                                        : orderBookToSell_.erase(order);
    mUsers.find(std::stoi(order.userID))->second.orders.erase(order);
    return "-->Cancel order to " +
           std::string(order.type == common::OrderType_Buy ? "buy " : "sell ") +
           std::to_string(order.volume.second) + order.volume.first + " for " +
           std::to_string(order.price.second) + order.price.first +
           " apiece accepted";
  }
  return user.name;
}

/**
 * @brief Снять денежные средства.
 * @param aUserId ID пользователя.
 * @param currencyTypeValue Тип валюты-значение.
 * @return Результат снятия денежных средств.
 */
std::string TradingExchangeClient::withdraw(
    const std::string &aUserId,
    const common::CurrencyTypeValue &currencyTypeValue) {
  User user = getUser(aUserId);
  if (user.name != "Unknown User") {
    mUsers.find(std::stoi(aUserId))->second.balance[currencyTypeValue.first] -=
        currencyTypeValue.second;
    return "-->Withdraw " + std::to_string(currencyTypeValue.second) +
           currencyTypeValue.first + " accepted";
  }
  return user.name;
}

/**
 * @brief Внести денежные средства.
 * @param aUserId ID пользователя.
 * @param currencyTypeValue Тмп валюты-значение.
 * @return Результат внесения денежных средств.
 */
std::string TradingExchangeClient::deposit(
    const std::string &aUserId,
    const common::CurrencyTypeValue &currencyTypeValue) {
  User user = getUser(aUserId);
  if (user.name != "Unknown User") {
    mUsers.find(std::stoi(aUserId))->second.balance[currencyTypeValue.first] +=
        currencyTypeValue.second;
    return "-->Deposit " + std::to_string(currencyTypeValue.second) +
           currencyTypeValue.first + " accepted";
  }
  return user.name;
}
