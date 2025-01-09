#include "Common.hpp"
#include "json.hpp"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <queue>
#include <set>

using boost::asio::ip::tcp;

/**
 * @brief Клиент торговой биржи.
 * @details Регистрирует новых пользователей, добавляет заявки на
 * покупку/продажу и возвращает баланс пользователя.
 */
class TradingExchangeClient {
public:
  Order parseOrderMessage(std::string str) {
    Order order;
    nlohmann::json j = nlohmann::json::parse(str);
    order.volume.first = j["volume"]["currencyType"].get<std::string>();
    order.volume.second = std::stof(j["volume"]["value"].get<std::string>());
    order.price.first = j["price"]["currencyType"].get<std::string>();
    order.price.second = std::stof(j["price"]["value"].get<std::string>());
    std::cout << order.price.second << std::endl;
    order.time = std::time_t(std::stol(j["time"].get<std::string>()));
    order.type =
        j["type"].get<std::string>() == "Buy" ? OrderType_Buy : OrderType_Sell;
    return order;
  }

  std::string createBalanceMessage(const User &user) {
    std::string balanceMessage;
    for (const auto &currencyTypeValue : user.balance) {
      balanceMessage += (std::to_string(currencyTypeValue.second) +
                         currencyTypeValue.first + '\n');
    }
    return balanceMessage;
  }

  std::string createOrdersMessage(const User &user) {
    if (user.orders.empty()) {
      return "No orders";
    }
    nlohmann::json jsonMessage;
    std::size_t num = 0;
    for (const auto &order : user.orders) {
      ++num;
      jsonMessage[std::to_string(num)] = {
          {"volume",
           {{"currencyType", order.volume.first},
            {"value", order.volume.second}}},
          {"price",
           {{"currencyType", order.price.first},
            {"value", order.price.second}}},
          {"type", order.type},
          {"time", order.time}};
    }
    jsonMessage["count"] = num;
    return jsonMessage.dump();
  }

  // Компаратор для заявок на покупку
  struct CompareBuy {
    bool operator()(const Order &a, const Order &b) const {
      // Высокие цены и раннее время регистрации должны быть выше
      if (a.price.second != b.price.second) {
        return a.price.second > b.price.second; // Большая цена - выше
      }
      return a.time > b.time; // Раннее время регистрации - выше
    }
  };

  // Компаратор для заявок на продажу
  struct CompareSell {
    bool operator()(const Order &a, const Order &b) const {
      // Низкие цены и раннее время регистрации должны быть выше
      if (a.price.second != b.price.second) {
        return a.price.second < b.price.second; // Меньшая цена - выше
      }
      return a.time > b.time; // Раннее время регистрации - выше
    }
  };

  // Компаратор для заявок
  struct Compare {
    bool operator()(const Order &a, const Order &b) const {
      return a.time > b.time; // Раннее время регистрации - выше
    }
  };

  /**
   * @brief Пользователь.
   */
  struct User {
    std::string name = "Unknown User"; //!< Имя.
    Balance balance;                   //!< Баланс.
    std::set<Order, Compare> orders;   //!< Заявки.
  };

  /**
   * @brief Тип заявки.
   */
  enum OrderType {
    OrderType_None, //!< Нет заявки.
    OrderType_Buy,  //!< Заявка на покупку.
    OrderType_Sell  //!< Заявка на продажу.
  };

  typedef std::map<std::string, float> Balance; //<! Баланс.
  //! Пара тип валюты - значение.
  typedef std::pair<std::string, float> CurrencyTypeValue;

  /**
   * @brief Заявка.
   */
  struct Order {
    std::string userID; //!< ID пользователя.
    //! Объём заявки (сколько необходимо купить валюты).
    CurrencyTypeValue volume = CurrencyTypeValue("", 0.f);
    //! Цена покупаемой валюты.
    CurrencyTypeValue price = CurrencyTypeValue("", 0.f);
    OrderType type = OrderType_None; //!< Тип заявки.
    std::time_t time = 0; //!< Время регистрации заявки.
  };

  /**
   * @brief Обработка заявок в "стакане".
   */
  void process() { matchOrders(); }

  void matchOrders() {
    if (orderBookToBuy_.empty() || orderBookToSell_.empty()) {
      return;
    }

    Order orderToSell = *orderBookToSell_.begin();
    Order orderToBuy = *orderBookToBuy_.begin();
    CurrencyTypeValue price(orderToSell.price.first, 0.f);
    CurrencyTypeValue volume(orderToSell.volume.first, 0.f);

    if (orderToBuy.price.second >= orderToSell.price.second) {
      CancelOrder(orderToSell);
      CancelOrder(orderToBuy);
      if (orderToSell.volume.second > orderToBuy.volume.second) {
        orderToSell.volume.second -= orderToBuy.volume.second;
        price.second = orderToBuy.price.second * orderToBuy.volume.second;
        volume.second = orderToBuy.volume.second;
        RegisterOrder(orderToSell);
      } else if (orderToSell.volume.second < orderToBuy.volume.second) {
        orderToBuy.volume.second -= orderToSell.volume.second;
        volume.second = orderToSell.volume.second;
        price.second = orderToBuy.price.second * orderToSell.volume.second;
        RegisterOrder(orderToBuy);
      }
      Withdraw(orderToSell.userID, volume);
      Deposit(orderToBuy.userID, volume);

      std::cout << orderToBuy.price.second << " " << price.second << std::endl;
      Deposit(orderToSell.userID, price);
      Withdraw(orderToBuy.userID, price);
    }
  }
  /**
   * @brief Зарегистрировать нового пользователя.
   * @param aUserName Имя пользователя.
   * @return ID нового пользователя.
   */
  std::string RegisterNewUser(const std::string &aUserName) {
    size_t newUserId = mUsers.size();
    mUsers[newUserId].name = aUserName;
    for (const std::string currency : currencies) {
      mUsers[newUserId].balance[currency] = 0.f;
    }
    return std::to_string(newUserId);
  }

  /**
   * @brief Получить информацию о клиенте.
   * @param aUserId ID пользователя.
   * @return Пользователь.
   */
  User GetUser(const std::string &aUserId) {
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
  std::string RegisterOrder(const Order &order) {
    User user = GetUser(order.userID);
    if (user.name != "Unknown User") {
      order.type == OrderType_Buy ? orderBookToBuy_.insert(order)
                                  : orderBookToSell_.insert(order);
      mUsers.find(std::stoi(order.userID))->second.orders.insert(order);
      std::cout << mUsers.find(std::stoi(order.userID))->second.orders.size()
                << std::endl;
      return "-->Order to " +
             std::string(order.type == OrderType_Buy ? "buy " : "sell ") +
             std::to_string(order.volume.second) + order.volume.first +
             " for " + std::to_string(order.price.second) + order.price.first +
             " apiece accepted";
    }
    return user.name;
  }

  /**
   * @brief Отменить заявку на покупку/продажу.
   * @param order Заявка.
   * @return Результат отмены заявки.
   */
  std::string CancelOrder(const Order &order) {
    User user = GetUser(order.userID);
    if (user.name != "Unknown User") {
      order.type == OrderType_Buy ? orderBookToBuy_.erase(order)
                                  : orderBookToSell_.erase(order);
      mUsers.find(std::stoi(order.userID))->second.orders.erase(order);
      return "-->Cancel order to " +
             std::string(order.type == OrderType_Buy ? "buy " : "sell ") +
             std::to_string(order.volume.second) + order.volume.first +
             " for " + std::to_string(order.price.second) + order.price.first +
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
  std::string Withdraw(const std::string &aUserId,
                       const CurrencyTypeValue &currencyTypeValue) {
    User user = GetUser(aUserId);
    if (user.name != "Unknown User") {
      mUsers.find(std::stoi(aUserId))
          ->second.balance[currencyTypeValue.first] -= currencyTypeValue.second;
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
  std::string Deposit(const std::string &aUserId,
                      const CurrencyTypeValue &currencyTypeValue) {
    User user = GetUser(aUserId);
    if (user.name != "Unknown User") {
      mUsers.find(std::stoi(aUserId))
          ->second.balance[currencyTypeValue.first] += currencyTypeValue.second;
      return "-->Deposit " + std::to_string(currencyTypeValue.second) +
             currencyTypeValue.first + " accepted";
    }
    return user.name;
  }

private:
  //! Пользователи биржи.
  std::map<size_t, User> mUsers;
  //! Таблица заявок на покупку.
  std::set<Order, CompareBuy> orderBookToBuy_;
  //! Таблица заявок на продажу.
  std::set<Order, CompareSell> orderBookToSell_;
};