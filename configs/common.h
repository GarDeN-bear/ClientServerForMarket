#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <queue>
#include <set>

namespace limits {
const size_t buffSize = 1024;
}

static short port = 5555;

namespace common {

namespace requests {
const std::string SignIn = "SignIn";
const std::string SignUp = "SignUp";
const std::string Buy = "Buy";
const std::string Sell = "Sell";
const std::string Balance = "Balance";
const std::string Deposit = "Deposit";
const std::string Withdraw = "Withdraw";
const std::string Orders = "Orders";
const std::string Cancel = "Cancel";
} // namespace requests

std::set<std::string> currencyPairs = {"RU-USD", "USD-RU"};
std::set<std::string> currencyTypes = {"RU", "USD"};

typedef std::map<std::string, float> Balance; //<! Баланс.
//! Пара тип валюты - значение.
typedef std::pair<std::string, float> CurrencyTypeValue;

/**
 * @brief Тип заявки.
 */
enum OrderType {
  OrderType_None, //!< Нет заявки.
  OrderType_Buy,  //!< Заявка на покупку.
  OrderType_Sell  //!< Заявка на продажу.
};

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

} // namespace common
