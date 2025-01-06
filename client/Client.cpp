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

std::string createOrderMessage(const std::string volumeCurrencyType,
                               const std::string volume,
                               const std::string priceCurrencyType,
                               const std::string price) {
  return "VolumeCurrencyType:" + volumeCurrencyType + ";\nVolume:" + volume +
         ";\nPriceCurrencyType:" + priceCurrencyType + ";\nPrice:" + price +
         ";\n";
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

std::string createOrder() {
  std::cout << "Enabled for trading currency types:" << std::endl;
  for (const std::string &currency : currencies) {
    std::cout << currency << std::endl;
  }
  std::cout << "\nInput volume currency type:" << std::endl;
  std::string volumeCurrencyType;
  std::cin >> volumeCurrencyType;
  if (currencies.find(volumeCurrencyType) == currencies.end()) {
    std::cout << "Wrong currency type\n" << std::endl;
    return "";
  }
  std::cout << "Input volume:" << std::endl;
  std::string volume;
  std::cin >> volume;
  if (!is_valid_float(volume)) {
    std::cout << "Wrong volume value\n" << std::endl;
    return "";
  }
  std::cout << "Input price currency type:" << std::endl;
  std::string priceCurrencyType;
  std::cin >> priceCurrencyType;
  if (currencies.find(priceCurrencyType) == currencies.end()) {
    std::cout << "Wrong currency type\n" << std::endl;
    return "";
  }
  std::cout << "Input price:" << std::endl;
  std::string price;
  std::cin >> price;
  if (!is_valid_float(price)) {
    std::cout << "Wrong price value\n" << std::endl;
    return "";
  }
  return createOrderMessage(volumeCurrencyType, volume, priceCurrencyType,
                            price);
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
                   "4) Exit\n"
                << std::endl;

      short menu_option_num;
      std::cin >> menu_option_num;
      std::cout << menu_option_num << std::endl;
      switch (menu_option_num) {
      case 1: {
        const std::string order = createOrder();
        if (order.empty()) {
          break;
        }

        SendMessage(s, my_id, Requests::Buy, order);
        std::cout << ReadMessage(s) << std::endl;
        break;
      }
      case 2: {
        const std::string order = createOrder();
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