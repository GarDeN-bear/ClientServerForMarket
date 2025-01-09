#include "trading_exchange_client.h"

/**
 * @brief Глобавльное хранилище пользователей и заявок в "стакане".
 */
inline TradingExchangeClient &GetTradingExchangeClient() {
  static TradingExchangeClient tradingExchangeClient;
  return tradingExchangeClient;
}