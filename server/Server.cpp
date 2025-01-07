#include "Common.hpp"
#include "json.hpp"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <cstdlib>
#include <iostream>
#include <queue>
#include <set>

using boost::asio::ip::tcp;

std::set<std::string> currencies = {"RU", "USD"};

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
  std::uint64_t time = 0; //!< Время регистрации заявки.
};

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

CurrencyTypeValue parseTypeValueMessage(std::string str) {
  std::string first = ":";
  std::string last = ";";
  std::size_t firstIndex = str.find(first);
  std::size_t lastIndex = str.find(last);
  CurrencyTypeValue currencyTypeValue;
  currencyTypeValue.first =
      str.substr(firstIndex + 1, lastIndex - firstIndex - 1);
  str.erase(firstIndex, 1);
  str.erase(lastIndex - 1, 1);
  firstIndex = str.find(first);
  lastIndex = str.find(last);
  currencyTypeValue.second =
      std::stof(str.substr(firstIndex + 1, lastIndex - firstIndex - 1));
  return currencyTypeValue;
}

void parseOrderMessage(Order &order, std::string str) {
  std::string sep = "|";
  order.volume = parseTypeValueMessage(str);
  order.price = parseTypeValueMessage(str.erase(0, str.find(sep)));
}

std::string createBalanceMessage(const User &user) {
  std::string balanceMessage;
  for (const auto &currencyTypeValue : user.balance) {
    balanceMessage += (std::to_string(currencyTypeValue.second) +
                       currencyTypeValue.first + '\n');
  }
  return balanceMessage;
}

/**
 * @brief Класс хранилища пользователей и заявок в "стакане".
 * @details Регистрирует новых пользователей, добавляет заявки на
 * покупку/продажу и возвращает баланс пользователя.
 */
class Core {
public:
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

/**
 * @brief Глобавльное хранилище пользователей и заявок в "стакане".
 */
inline Core &GetCore() {
  static Core core;
  return core;
}

class session {
public:
  session(boost::asio::io_service &io_service) : socket_(io_service) {}

  tcp::socket &socket() { return socket_; }

  void start() {
    socket_.async_read_some(
        boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
  }

  // Обработка полученного сообщения.
  void handle_read(const boost::system::error_code &error,
                   size_t bytes_transferred) {
    if (!error) {
      data_[bytes_transferred] = '\0';

      // Парсим json, который пришёл нам в сообщении.
      auto j = nlohmann::json::parse(data_);
      auto reqType = j["ReqType"];

      std::string reply = "Error! Unknown request type";
      if (reqType == Requests::Registration) {
        // Это реквест на регистрацию пользователя.
        // Добавляем нового пользователя и возвращаем его ID.
        reply = GetCore().RegisterNewUser(j["Message"]);
      } else if (reqType == Requests::Buy) {
        // Это реквест на регистрацию заявки на покупку.
        // Добавляем заявку пользователя в "стакан".
        Order order;
        parseOrderMessage(order, j["Message"]);
        order.type = OrderType_Buy;
        order.userID = j["UserId"];
        reply = GetCore().RegisterOrder(order);
      } else if (reqType == Requests::Sell) {
        // Это реквест на регистрацию заявки на продажу.
        // Добавляем заявку пользователя в "стакан".
        Order order;
        parseOrderMessage(order, j["Message"]);
        order.type = OrderType_Sell;
        order.userID = j["UserId"];
        reply = GetCore().RegisterOrder(order);
      } else if (reqType == Requests::Balance) {
        // Это реквест на получение баланса пользователя.
        reply = createBalanceMessage(GetCore().GetUser(j["UserId"]));
      } else if (reqType == Requests::Deposit) {
        // Это реквест на внесение денежный средств.
        reply =
            GetCore().Deposit(j["UserId"], parseTypeValueMessage(j["Message"]));
      } else if (reqType == Requests::Withdraw) {
        // Это реквест на снятие денежных средств.
        reply = GetCore().Withdraw(j["UserId"],
                                   parseTypeValueMessage(j["Message"]));
      }

      boost::asio::async_write(socket_,
                               boost::asio::buffer(reply, reply.size()),
                               boost::bind(&session::handle_write, this,
                                           boost::asio::placeholders::error));
    } else {
      delete this;
    }
  }

  void handle_write(const boost::system::error_code &error) {
    if (!error) {
      socket_.async_read_some(
          boost::asio::buffer(data_, max_length),
          boost::bind(&session::handle_read, this,
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));
    } else {
      delete this;
    }
  }

private:
  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

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