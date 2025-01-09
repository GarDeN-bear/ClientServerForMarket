#include "trading_exchange_client.h"

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
  std::time_t time = 0; //!< Время регистрации заявки.
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
         {{"currencyType", order.price.first}, {"value", order.price.second}}},
        {"type", order.type},
        {"time", order.time}};
  }
  jsonMessage["count"] = num;
  return jsonMessage.dump();
}

/**
 * @brief Глобавльное хранилище пользователей и заявок в "стакане".
 */
inline TradingExchangeClient &GetTradingExchangeClient() {
  static TradingExchangeClient TradingExchangeClient;
  return TradingExchangeClient;
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
        reply = GetTradingExchangeClient().RegisterNewUser(j["Message"]);
      } else if (reqType == Requests::Buy) {
        // Это реквест на регистрацию заявки на покупку.
        // Добавляем заявку пользователя в "стакан".
        Order order = parseOrderMessage(j["Message"]);
        order.userID = j["UserId"];
        reply = GetTradingExchangeClient().RegisterOrder(order);
      } else if (reqType == Requests::Sell) {
        // Это реквест на регистрацию заявки на продажу.
        // Добавляем заявку пользователя в "стакан".
        Order order = parseOrderMessage(j["Message"]);
        order.userID = j["UserId"];
        reply = GetTradingExchangeClient().RegisterOrder(order);
      } else if (reqType == Requests::Balance) {
        // Это реквест на получение баланса пользователя.
        reply = createBalanceMessage(
            GetTradingExchangeClient().GetUser(j["UserId"]));
      } else if (reqType == Requests::Deposit) {
        // Это реквест на внесение денежный средств.
        reply = GetTradingExchangeClient().Deposit(
            j["UserId"], parseTypeValueMessage(j["Message"]));
      } else if (reqType == Requests::Withdraw) {
        // Это реквест на снятие денежных средств.
        reply = GetTradingExchangeClient().Withdraw(
            j["UserId"], parseTypeValueMessage(j["Message"]));
      } else if (reqType == Requests::Orders) {
        // Это реквест на получение списка заявок пользователя.
        reply = createOrdersMessage(
            GetTradingExchangeClient().GetUser(j["UserId"]));
      } else if (reqType == Requests::Cancel) {
        // Это реквест на отмену заявки.
        // Order order;
        // parseOrderMessage(order, j["Message"]);
        // order.userID = j["UserId"];
        // reply = GetTradingExchangeClient().CancelOrder(order);
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
    GetTradingExchangeClient().process();

    // Перезапуск таймера
    timer_.expires_at(timer_.expiry() + boost::asio::chrono::seconds(1));
    start_timer();
  }
};
