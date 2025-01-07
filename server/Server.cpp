#include "Common.hpp"
#include "json.hpp"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <cstdlib>
#include <iostream>
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
  //! Объём заявки (сколько необходимо купить валюты).
  CurrencyTypeValue volume = CurrencyTypeValue("", 0.f);
  //! Цена покупаемой валюты.
  CurrencyTypeValue price = CurrencyTypeValue("", 0.f);
  OrderType type = OrderType_None; //!< Тип заявки.
  std::uint64_t time = 0; //!< Время регистрации заявки.
};

typedef std::vector<Order> Orders; //!< Заявки.

/**
 * @brief Пользователь.
 */
struct User {
  std::string name = "Unknown User"; //!< Имя.
  Balance balance;                   //!< Баланс.
  Orders orders;                     //!< Заявки.
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
   * @param aUserId ID пользователя.
   * @param order Заявка.
   * @return Результат регистрации заявки.
   */
  std::string RegisterOrder(const std::string &aUserId, const Order &order) {
    User user = GetUser(aUserId);
    if (user.name != "Unknown User") {
      depthOfMarket_.push_back(order);
      mUsers.find(std::stoi(aUserId))->second.orders.push_back(order);
      return "-->Order to " +
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
   * @param currencyTypeValue Тмп валюты-значение.
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
  //! Таблица лимитных заявок ("стакан").
  Orders depthOfMarket_;
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
        reply = GetCore().RegisterOrder(j["UserId"], order);
      } else if (reqType == Requests::Sell) {
        // Это реквест на регистрацию заявки на продажу.
        // Добавляем заявку пользователя в "стакан".
        Order order;
        parseOrderMessage(order, j["Message"]);
        order.type = OrderType_Sell;
        reply = GetCore().RegisterOrder(j["UserId"], order);
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
        acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
    std::cout << "Server started! Listen " << port << " port" << std::endl;

    session *new_session = new session(io_service_);
    acceptor_.async_accept(new_session->socket(),
                           boost::bind(&server::handle_accept, this,
                                       new_session,
                                       boost::asio::placeholders::error));
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