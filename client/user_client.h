#pragma once

#include "common.h"

using boost::asio::ip::tcp;

/**
 * @brief Клиент пользователя.
 * @details Выполняет подключение к серверу и взаимодействие пользователя с
 * сервером.
 *
 */
class UserClient {
public:
  /**
   * @brief Конструктор.
   * @param address Адрес.
   * @param port Порт.
   * @param io_service Сервис для сетевых операций.
   * @details Выполняется подключение к серверу.
   */
  UserClient(const std::string &address, const uint16_t port,
             boost::asio::io_service &io_service);

  /**
   * @details Отправляет запросы в сервер и принимает ответы от сервера.
   */
  void process();

private:
  std::string address_; //!< Адрес.
  uint16_t port_;       //!< Порт.
  tcp::socket s_;       //!< Сокет.
  std::string myId_;    //!< ID клиента.

  /**
   * @details Войти в аккаунт.
   * @return ID клиента.
   */
  std::string signIn();

  /**
   * @details Зарегистрировать аккаунт.
   * @return ID клиента.
   */
  std::string signUp();

  /**
   * @brief Отправить запрос на сервер.
   * @param requestType Тип запроса.
   * @param request Запрос.
   */
  void sendRequest(const std::string &requestType, const std::string &request);

  /**
   * @brief Получить ответ от сервера.
   * @return Ответ от сервера.
   */
  std::string receiveResponse();

  /**
   * @brief Переключить опцию меню.
   * @details Отправляет запрос на сервер.
   */
  void switchMenuOption();

  /**
   * @brief Создать запрос на создание заявки.
   * @param orderType Тип заявки.
   * @return Ответ от сервера.
   */
  std::string createOrderRequest(const common::OrderType &orderType);

  /**
   * @brief Показать валютные пары.
   */
  void showCurrencyPairs();

  /**
   * @brief Ввести валютную пару.
   * @return Валютная пара.
   */
  std::string inputCurrencyPair();

  /**
   * @brief Показать доступные валюты.
   */
  void showCurrencyTypes();

  /**
   * @brief Ввести валюту.
   * @return Валюта.
   */
  std::string inputCurrencyType();

  /**
   * @brief Ввести значение валюты.
   * @return Значение.
   */
  std::string inputCurrencyValue();

  /**
   * @brief Разделить валютную пару.
   * @param currencyPair Валютная пара.
   * @return Разделенная валютная пара.
   */
  std::pair<std::string, std::string>
  separateCurrencyPair(const std::string &currencyPair);

  /**
   * @brief Валидно ли преобразование строки к float.
   * @param str Строка с значением.
   * @return true-да, false-нет.
   */
  bool isValidFloat(const std::string &str);

  /**
   * @brief Функция для создания JSON строки из OrderRequest.
   * @param request Запрос.
   * @return JSON строка.
   */
  std::string orderRequestToJson(const common::Order &request);

  /**
   * @brief Парсить сообщение с заявками пользователя.
   * @param msg Сообщение.
   */
  void parseOrdersMessage(const std::string &msg);

  /**
   * @brief Перевести пару тип валюты-значение в JSON строку.
   * @param pair Пара.
   * @return JSON строка.
   */
  std::string convertTypeValuePairToJSON(const common::CurrencyTypeValue &pair);
};