#pragma once

#include "common.h"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <queue>
#include <set>

using boost::asio::ip::tcp;

/**
 * @brief  Класс сессии связи между сервером и клиентом.
 * @details Обрабатывает входящие запросы, выполняет соответствующие действия и
 * отправляет ответы клиенту.
 */
class Session {
public:
  /**
   * @brief Конструктор.
   * @param io_service Сервис для сетевых операций.
   */
  Session(boost::asio::io_service &io_service);

  /**
   * @brief Получить сокет.
   * @return Сокет.
   */
  tcp::socket &getSocket();

  /**
   * @brief Запустить работу сессии.
   */
  void startSession();

  /**
   * @brief Обработать входящие данные.
   * @param error Ошибка чтения данных.
   * @param bytes_transferred Количество принятых байт.
   */
  void handleRead(const boost::system::error_code &error,
                  size_t bytes_transferred);

  /**
   * @brief Обработать отправление данных.
   * @param error Ошибка чтения данных.
   */
  void handleWrite(const boost::system::error_code &error);

private:
  tcp::socket socket_;          //!< Сокет.
  char data_[limits::buffSize]; //!< Принимаемые данные.

  /**
   * @brief Создать заявку из запроса.
   * @param request Запрос.
   * @return Заявка.
   */
  common::Order createOrderFromRequest(const std::string &request) const;

  /**
   * @brief Создать ответ на запрос.
   * @param json Данные в JSON формате.
   * @return Ответ.
   */
  std::string createResponse(const nlohmann::json &json);
};