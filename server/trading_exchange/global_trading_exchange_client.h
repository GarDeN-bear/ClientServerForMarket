#include "trading_exchange_client.h"

/**
 * @brief Глобавльный объект торговой биржи.
 */
inline TradingExchangeClient &GlobalTradingExchangeClient() {
  static TradingExchangeClient tradingExchangeClient;
  return tradingExchangeClient;
}