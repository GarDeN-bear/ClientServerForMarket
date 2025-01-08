#include <boost/asio.hpp>
#include <iostream>

#include "Common.hpp"
#include "json.hpp"
#include <set>

using boost::asio::ip::tcp;
std::set<std::string> currencies = {"RU", "USD"};

// Отправка сообщения на сервер по шаблону.
void SendMessage(tcp::socket &aSocket, const std::string &aId,
                 const std::string &aRequestType, const std::string &aMessage) {
  nlohmann::json req;
  req["UserId"] = aId;
  req["ReqType"] = aRequestType;
  req["Message"] = aMessage;

  std::string request = req.dump();
  boost::asio::write(aSocket, boost::asio::buffer(request, request.size()));
}

// Возвращает строку с ответом сервера на последний запрос.
std::string ReadMessage(tcp::socket &aSocket) {
  boost::asio::streambuf b;
  boost::asio::read_until(aSocket, b, "\0");
  std::istream is(&b);
  std::string line(std::istreambuf_iterator<char>(is), {});
  return line;
}

// "Создаём" пользователя, получаем его ID.
std::string ProcessRegistration(tcp::socket &aSocket) {
  std::string name;
  std::cout << "Hello! Enter your name: ";
  std::cin >> name;

  // Для регистрации Id не нужен, заполним его нулём
  SendMessage(aSocket, "0", Requests::Registration, name);
  return ReadMessage(aSocket);
}

std::string createTypeValuePairMessage(const std::string &currencyType,
                                       const std::string &currencyValue) {
  return "CurrencyType:" + currencyType + ";\nCurrencyValue:" + currencyValue +
         ";";
}
// Функция для проверки, можно ли перевести строку в float
bool is_valid_float(const std::string &str) {
  try {
    float value = std::stof(str);
    return value >= 0.f;
  } catch (const std::invalid_argument &e) {
    return false;
  } catch (const std::out_of_range &e) {
    return false;
  }
}
std::string inputCurrencyType() {
  std::cout << "Input currency type:" << std::endl;
  std::string currencyType;
  std::cin >> currencyType;
  if (currencies.find(currencyType) == currencies.end()) {
    return "";
  }
  return currencyType;
}

/**
 * @brief Тип заявки.
 */
enum OrderType {
  OrderType_None, //!< Нет заявки.
  OrderType_Buy,  //!< Заявка на покупку.
  OrderType_Sell  //!< Заявка на продажу.
};

void parseOrdersMessage(const std::string &str) {
  nlohmann::json j = nlohmann::json::parse(str);
  const std::size_t num = j["count"].get<std::size_t>();
  std::cout << "Count opened orders: " + std::to_string(num) << std::endl;
  for (size_t i = 0; i < num; ++i) {
    std::cout << "Order №" << i + 1 << ":" << std::endl;
    std::cout << "Order type: "
              << (j[std::to_string(i + 1)]["type"].get<OrderType>() ==
                          OrderType_Buy
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

std::string inputCurrencyValue() {
  std::cout << "Input currency value:" << std::endl;
  std::string currencyValue;
  std::cin >> currencyValue;
  if (!is_valid_float(currencyValue)) {
    return "";
  }
  return currencyValue;
}

void showCurrencyTypes() {
  std::cout << "Enabled for trading currency types:" << std::endl;
  for (const std::string &currency : currencies) {
    std::cout << currency << std::endl;
  }
}
struct OrderMessage {
  std::pair<std::string, std::string> volume;
  std::pair<std::string, std::string> price;
  std::string type;
  std::string time;
};

// Функция для создания JSON строки из OrderMessage
std::string orderMessageToJson(const OrderMessage &message) {
  nlohmann::json jsonMessage;
  jsonMessage["volume"] = {{"currencyType", message.volume.first},
                           {"value", message.volume.second}};
  jsonMessage["price"] = {{"currencyType", message.price.first},
                          {"value", message.price.second}};
  jsonMessage["type"] = message.type;
  jsonMessage["time"] = message.time;

  return jsonMessage.dump();
}

std::string createOrder(const std::string &orderType) {
  showCurrencyTypes();
  std::cout << "\nInput volume" << std::endl;
  std::string volumeCurrencyType = inputCurrencyType();
  if (volumeCurrencyType.empty()) {
    std::cout << "Wrong currency type\n" << std::endl;
    return "";
  }
  std::string volume = inputCurrencyValue();
  if (volume.empty()) {
    std::cout << "Wrong volume value\n" << std::endl;
    return "";
  }
  std::cout << "\nInput price" << std::endl;
  std::string priceCurrencyType = inputCurrencyType();
  if (priceCurrencyType.empty()) {
    std::cout << "Wrong currency type\n" << std::endl;
    return "";
  }
  std::string price = inputCurrencyValue();
  if (price.empty()) {
    std::cout << "Wrong price value\n" << std::endl;
    return "";
  }
  std::time_t time =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  OrderMessage message;
  message.volume.first = volumeCurrencyType;
  message.volume.second = volume;
  message.price.first = priceCurrencyType;
  message.price.second = price;
  message.type = orderType;
  message.time = std::to_string(time);
  return orderMessageToJson(message);
}

int main() {
  try {
    boost::asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(tcp::v4(), "127.0.0.1", std::to_string(port));
    tcp::resolver::iterator iterator = resolver.resolve(query);

    tcp::socket s(io_service);
    s.connect(*iterator);

    // Мы предполагаем, что для идентификации пользователя будет использоваться
    // ID. Тут мы "регистрируем" пользователя - отправляем на сервер имя, а
    // сервер возвращает нам ID. Этот ID далее используется при отправке
    // запросов.
    std::string my_id = ProcessRegistration(s);

    while (true) {
      // Тут реализовано "бесконечное" меню.
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

      short menu_option_num;
      std::cin >> menu_option_num;
      std::cout << menu_option_num << std::endl;
      switch (menu_option_num) {
      case 1: {
        const std::string order = createOrder(Requests::Buy);
        if (order.empty()) {
          break;
        }

        SendMessage(s, my_id, Requests::Buy, order);
        std::cout << ReadMessage(s) << std::endl;
        break;
      }
      case 2: {
        const std::string order = createOrder(Requests::Sell);
        if (order.empty()) {
          break;
        }

        SendMessage(s, my_id, Requests::Sell, order);
        std::cout << ReadMessage(s) << std::endl;
        break;
      }
      case 3: {
        SendMessage(s, my_id, Requests::Balance, "");
        std::cout << ReadMessage(s) << std::endl;
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
        SendMessage(s, my_id, Requests::Deposit,
                    createTypeValuePairMessage(depositCurrencyType,
                                               depositCurrencyValue));
        std::cout << ReadMessage(s) << std::endl;
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
        SendMessage(s, my_id, Requests::Withdraw,
                    createTypeValuePairMessage(withdrawCurrencyType,
                                               withdrawCurrencyValue));
        std::cout << ReadMessage(s) << std::endl;
        break;
      }
      case 6: {
        SendMessage(s, my_id, Requests::Orders, "");
        std::string message = ReadMessage(s);
        if (message == "No orders") {
          break;
        }
        parseOrdersMessage(message);
        break;
      }
      case 7: {
        SendMessage(s, my_id, Requests::Orders, "");
        std::string orders = ReadMessage(s);
        std::cout << orders << std::endl;
        if (orders == "No orders") {
          break;
        }
        std::cout << "Input order num:\n";
        std::size_t num;
        std::cin >> num;
        SendMessage(s, my_id, Requests::Cancel, "");
        std::cout << ReadMessage(s) << std::endl;
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
  } catch (std::exception &e) {
    std::cerr << "Client exception: " << e.what() << "\n";
  }

  return 0;
}