#include "user_client.h"

UserClient::UserClient(const std::string &address, const uint16_t port,
                       boost::asio::io_service &io_service)
    : address_(address), port_(port), s_(io_service), myId_("") {
  try {
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(tcp::v4(), address_, std::to_string(port_));
    tcp::resolver::iterator iterator = resolver.resolve(query);

    tcp::socket s_(io_service);
    s_.connect(*iterator);
  } catch (std::exception &e) {
    std::cerr << "UserClient::UserClient exception: " << e.what() << "\n";
  }
}

void UserClient::process() {
  try {
    while (myId_.empty()) {
      // Мы предполагаем, что для идентификации пользователя будет
      // использоваться ID. Тут мы "регистрируем" пользователя - отправляем на
      // сервер имя, а сервер возвращает нам ID. Этот ID далее используется при
      // отправке запросов.
      std::cout << "Menu:\n"
                   "1) Sign in\n"
                   "2) Sign up\n"
                   "3) Exit\n"
                << std::endl;
      switchMenuOption();
    }
    while (true) {
      std::cout << "Menu:\n"
                   "1) Buy\n"
                   "2) Sell\n"
                   "3) Balance\n"
                   "4) Deposit\n"
                   "5) Withdraw\n"
                   "6) Orders\n"
                   "7) Cancel\n"
                   "8) Exit\n"
                << std::endl;
      switchMenuOption();
    }
  } catch (std::exception &e) {
    std::cerr << "UserClient exception: " << e.what() << "\n";
  }
}

std::string UserClient::signIn() {
  std::string name;
  std::cout << "Hello! Enter your name: ";
  std::cin >> name;

  sendRequest(common::requests::SignIn, name);
  return receiveResponse();
}

std::string UserClient::signUp() {
  std::string name;
  std::cout << "Hello! Enter your name: ";
  std::cin >> name;

  sendRequest(common::requests::SignUp, name);
  return receiveResponse();
}

void UserClient::sendRequest(const std::string &requestType,
                             const std::string &message) {
  nlohmann::json req;
  req["UserId"] = myId_;
  req["ReqType"] = requestType;
  req["Message"] = message;

  std::string request = req.dump();
  boost::asio::write(s_, boost::asio::buffer(request, request.size()));
}

std::string UserClient::receiveResponse() {
  boost::asio::streambuf b;
  boost::asio::read_until(s_, b, "\0");
  std::istream is(&b);
  std::string line(std::istreambuf_iterator<char>(is), {});
  return line;
}

void UserClient::switchMenuOption() {
  short menuOptionNum;
  std::cin >> menuOptionNum;
  if (myId_.empty()) {
    switch (menuOptionNum) {
    case 1: {
      myId_ = signIn();
      break;
    }
    case 2: {
      myId_ = signUp();
      break;
    }
    case 3: {
      exit(0);
      break;
    }
    default: {
      std::cout << "Unknown menu option\n" << std::endl;
      break;
    }
    }
  } else {
    switch (menuOptionNum) {
    case 1: {
      const std::string orderRequest =
          createOrderRequest(common::requests::Buy);
      if (orderRequest.empty()) {
        break;
      }
      sendRequest(common::requests::Buy, orderRequest);
      std::cout << receiveResponse() << std::endl;
      break;
    }
    case 2: {
      const std::string orderRequest =
          createOrderRequest(common::requests::Sell);
      if (orderRequest.empty()) {
        break;
      }
      sendRequest(common::requests::Sell, orderRequest);
      std::cout << receiveResponse() << std::endl;
      break;
    }
    case 3: {
      sendRequest(common::requests::Balance, "");
      std::cout << receiveResponse() << std::endl;
      break;
    }
    case 4: {
      showCurrencyTypes();
      const std::string depositCurrencyType = inputCurrencyType();
      if (depositCurrencyType.empty()) {
        break;
      }
      const std::string depositCurrencyValue = inputCurrencyValue();
      if (depositCurrencyValue.empty()) {
        break;
      }
      std::pair<std::string, std::string> pair = {depositCurrencyType,
                                                  depositCurrencyValue};
      sendRequest(common::requests::Deposit, convertTypeValuePairToJSON(pair));
      std::cout << receiveResponse() << std::endl;
      break;
    }
    case 5: {
      showCurrencyTypes();
      const std::string withdrawCurrencyType = inputCurrencyType();
      if (withdrawCurrencyType.empty()) {
        break;
      }
      const std::string withdrawCurrencyValue = inputCurrencyValue();
      if (withdrawCurrencyValue.empty()) {
        break;
      }
      std::pair<std::string, std::string> pair = {withdrawCurrencyType,
                                                  withdrawCurrencyValue};
      sendRequest(common::requests::Withdraw, convertTypeValuePairToJSON(pair));
      std::cout << receiveResponse() << std::endl;
      break;
    }
    case 6: {
      sendRequest(common::requests::Orders, "");
      std::string message = receiveResponse();
      if (message == "No orders") {
        break;
      }
      parseOrdersMessage(message);
      break;
    }
    case 7: {
      sendRequest(common::requests::Orders, "");
      std::string orders = receiveResponse();
      std::cout << orders << std::endl;
      if (orders == "No orders") {
        break;
      }
      std::cout << "Input order num:\n";
      std::size_t num;
      std::cin >> num;
      sendRequest(common::requests::Cancel, "");
      std::cout << receiveResponse() << std::endl;
      break;
    }
    case 8: {
      exit(0);
      break;
    }
    default: {
      std::cout << "Unknown menu option\n" << std::endl;
      break;
    }
    }
  }
}

std::string UserClient::convertTypeValuePairToJSON(
    const std::pair<std::string, std::string> &pair) {
  nlohmann::json jsonStr;
  jsonStr["currencyType"] = pair.first;
  jsonStr["value"] = pair.second;
  return jsonStr.dump();
}

std::string UserClient::inputCurrencyPair() {
  std::cout << "Input currency pair:" << std::endl;
  std::string currencyPair;
  std::cin >> currencyPair;
  if (common::currencyPairs.find(currencyPair) == common::currencyPairs.end()) {
    return "";
  }
  return currencyPair;
}

void UserClient::showCurrencyTypes() {
  std::cout << "Currency types:" << std::endl;
  for (const std::string &type : common::currencyTypes) {
    std::cout << type << std::endl;
  }
}

std::string UserClient::inputCurrencyType() {
  std::cout << "Input currency type:" << std::endl;
  std::string currencyType;
  std::cin >> currencyType;
  if (common::currencyTypes.find(currencyType) == common::currencyTypes.end()) {
    return "";
  }
  return currencyType;
}

std::pair<std::string, std::string>
UserClient::separateCurrencyPair(const std::string &currencyPair) {
  char sep = '-';
  size_t i = currencyPair.find(sep);
  std::pair<std::string, std::string> pair;
  pair.first = currencyPair.substr(0, i);
  pair.second = currencyPair.substr(i + 1);
  return pair;
}

std::string UserClient::inputCurrencyValue() {
  std::cout << "Input currency value:" << std::endl;
  std::string currencyValue;
  std::cin >> currencyValue;
  if (!isValidFloat(currencyValue)) {
    return "";
  }
  return currencyValue;
}

std::string UserClient::createOrderRequest(const std::string &orderType) {
  showCurrencyPairs();
  std::cout << "\nInput currency pair" << std::endl;
  std::string currencyPair = inputCurrencyPair();
  if (currencyPair.empty()) {
    std::cout << "UserClient::createOrderRequest:Wrong currency pair\n"
              << std::endl;
    return "";
  }
  std::pair<std::string, std::string> separatedCurrencyPair =
      separateCurrencyPair(currencyPair);
  std::cout << "\nInput volume (" + separatedCurrencyPair.first + ")"
            << std::endl;
  std::string volume = inputCurrencyValue();
  if (volume.empty()) {
    std::cout << "UserClient::createOrderRequest:Wrong volume value\n"
              << std::endl;
    return "";
  }
  std::cout << "\nInput price (" + separatedCurrencyPair.second + ")"
            << std::endl;
  std::string price = inputCurrencyValue();
  if (price.empty()) {
    std::cout << "UserClient::createOrderRequest:Wrong price value\n"
              << std::endl;
    return "";
  }
  std::time_t time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

  OrderRequest request;
  request.volume.first = separatedCurrencyPair.first;
  request.volume.second = volume;
  request.price.first = separatedCurrencyPair.second;
  request.price.second = price;
  request.type = orderType;
  request.time = std::to_string(time);
  return orderRequestToJson(request);
}

void UserClient::showCurrencyPairs() {
  std::cout << "Currency pairs:" << std::endl;
  for (const std::string &pair : common::currencyPairs) {
    std::cout << pair << std::endl;
  }
}

bool UserClient::isValidFloat(const std::string &str) {
  try {
    float value = std::stof(str);
    return value >= 0.f;
  } catch (std::exception &e) {
    return false;
  }
}

std::string UserClient::orderRequestToJson(const OrderRequest &request) {
  nlohmann::json jsonStr;
  jsonStr["volume"] = {{"currencyType", request.volume.first},
                       {"value", request.volume.second}};
  jsonStr["price"] = {{"currencyType", request.price.first},
                      {"value", request.price.second}};
  jsonStr["type"] = request.type;
  jsonStr["time"] = request.time;
  return jsonStr.dump();
}

void UserClient::parseOrdersMessage(const std::string &msg) {
  nlohmann::json j = nlohmann::json::parse(msg);
  const std::size_t num = j["count"].get<std::size_t>();
  std::cout << "Count opened orders: " + std::to_string(num) << std::endl;
  for (size_t i = 0; i < num; ++i) {
    std::cout << "Order №" << i + 1 << ":" << std::endl;
    std::cout << "Order type: "
              << (j[std::to_string(i + 1)]["type"].get<common::OrderType>() ==
                          common::OrderType_Buy
                      ? "Buy"
                      : "Sell")
              << std::endl;
    std::time_t time;
    j[std::to_string(i + 1)]["time"].get_to(time);
    std::cout << "Registration time: " << std::ctime(&time);
    std::cout
        << "Volume: "
        << j[std::to_string(i + 1)]["volume"]["value"].get<float>() << " "
        << j[std::to_string(i + 1)]["volume"]["currencyType"].get<std::string>()
        << std::endl;
    std::cout
        << "Price: " << j[std::to_string(i + 1)]["price"]["value"].get<float>()
        << " "
        << j[std::to_string(i + 1)]["price"]["currencyType"].get<std::string>()
        << std::endl;
    std::cout << std::endl;
  }
}