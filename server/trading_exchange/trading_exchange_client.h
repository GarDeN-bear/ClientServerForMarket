#pragma once

#include "common.h"

/**
 * @brief Клиент торговой биржи.
 * @details Регистрирует новых пользователей, добавляет заявки на
 * покупку/продажу и возвращает баланс пользователя.
 */
class TradingExchangeClient {
private:
  /**
   *  @brief Компаратор для заявок на покупку.
   */
  struct CompareBuy {
    bool operator()(const common::Order &a, const common::Order &b) const {
      if (a.price.second != b.price.second) {
        return a.price.second > b.price.second;
      }
      return a.time > b.time;
    }
  };

  /**
   *  @brief Компаратор для заявок на продажу.
   */
  struct CompareSell {
    bool operator()(const common::Order &a, const common::Order &b) const {
      if (a.price.second != b.price.second) {
        return a.price.second < b.price.second;
      }
      return a.time > b.time;
    }
  };

  /**
   *  @brief Компаратор для заявок на покупку и продажу.
   */
  struct Compare {
    bool operator()(const common::Order &a, const common::Order &b) const {
      return a.time < b.time;
    }
  };

public:
  /**
   * @brief Пользователь.
   */
  struct User {
    std::string name = "Unknown User";       //!< Имя.
    std::string userId = "Unknown User";     //!< ID.
    common::Balance balance;                 //!< Баланс.
    std::set<common::Order, Compare> orders; //!< Заявки.
  };

  /**
   * @brief Обработка заявок в "стакане".
   */
  void process();

  /**
   * @brief Совместить заявки.
   */
  void matchOrders();

  /**
   * @brief Зарегистрировать нового пользователя.
   * @param userName Имя пользователя.
   * @return ID нового пользователя.
   */
  std::string registerNewUser(const std::string &userName);

  /**
   * @brief Получить информацию о клиенте.
   * @param userId ID пользователя.
   * @return Пользователь.
   */
  User getUserById(const std::string &userId);

  /**
   * @brief Получить информацию о клиенте.
   * @param userName Имя пользователя.
   * @return Пользователь.
   */
  User getUserByName(const std::string &userName);

  /**
   * @brief Зарегистрировать заявку на покупку/продажу.
   * @param order Заявка.
   * @return Результат регистрации заявки.
   */
  std::string registerOrder(const common::Order &order);
  /**
   * @brief Отменить заявку на покупку/продажу.
   * @param order Заявка.
   * @return Результат отмены заявки.
   */
  std::string cancelOrder(const common::Order &order);

  /**
   * @brief Снять денежные средства.
   * @param userId ID пользователя.
   * @param currencyTypeValue Тип валюты-значение.
   * @return Результат снятия денежных средств.
   */
  std::string withdraw(const std::string &userId,
                       const common::CurrencyTypeValue &currencyTypeValue);

  /**
   * @brief Внести денежные средства.
   * @param userId ID пользователя.
   * @param currencyTypeValue Тмп валюты-значение.
   * @return Результат внесения денежных средств.
   */
  std::string deposit(const std::string &userId,
                      const common::CurrencyTypeValue &currencyTypeValue);

private:
  //! Пользователи биржи.
  std::map<size_t, User> users;
  //! Таблица заявок на покупку.
  std::set<common::Order, CompareBuy> orderBookToBuy_;
  //! Таблица заявок на продажу.
  std::set<common::Order, CompareSell> orderBookToSell_;
};