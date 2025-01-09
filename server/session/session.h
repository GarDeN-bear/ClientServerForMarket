
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
        Order order = parseOrderMessage(j["Message"]);
        order.userID = j["UserId"];
        reply = GetCore().RegisterOrder(order);
      } else if (reqType == Requests::Sell) {
        // Это реквест на регистрацию заявки на продажу.
        // Добавляем заявку пользователя в "стакан".
        Order order = parseOrderMessage(j["Message"]);
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
      } else if (reqType == Requests::Orders) {
        // Это реквест на получение списка заявок пользователя.
        reply = createOrdersMessage(GetCore().GetUser(j["UserId"]));
      } else if (reqType == Requests::Cancel) {
        // Это реквест на отмену заявки.
        // Order order;
        // parseOrderMessage(order, j["Message"]);
        // order.userID = j["UserId"];
        // reply = GetCore().CancelOrder(order);
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