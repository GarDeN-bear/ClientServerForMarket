#pragma once

#include "common.h"
#include "session.h"

/**
 * @brief Сервер торговой биржи.
 */
class TradingExchangeServer {
public:
  /**
   * @brief Конструктор.
   * @param io_service Сервис для сетевых операций.
   * @details Выполняется запуск прослушивания входящих подключений и работы
   * биржи.
   */
  TradingExchangeServer(boost::asio::io_service &io_service);

  /**
   * @brief Обработать входящее подключение.
   * @param new_session Сессия связи между сервером и клиентом.
   * @param error Ошибка чтения данных.
   */
  void handleAccept(Session *new_session,
                    const boost::system::error_code &error);

private:
  boost::asio::io_service &io_service_; //!< Сервис для сетевых операций.
  tcp::acceptor acceptor_; //!< Объект для принятия входящих подключений.
  boost::asio::steady_timer timer_; //!< Таймер.

  /**
   * @brief Запустить таймер.
   */
  void startTimer();

  /**
   * @brief Вызывает обработчик торговой биржи.
   */
  void tradingExchangeProcess();
};
