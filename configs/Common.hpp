#ifndef CLIENSERVERECN_COMMON_HPP
#define CLIENSERVERECN_COMMON_HPP

#include <string>

static short port = 5555;

namespace Requests {
static std::string Registration = "Registration";
static std::string Buy = "Buy";
static std::string Sell = "Sell";
static std::string Balance = "Balance";
static std::string Deposit = "Deposit";
static std::string Withdraw = "Withdraw";
static std::string Orders = "Orders";
static std::string Cancel = "Cancel";
} // namespace Requests

#endif // CLIENSERVERECN_COMMON_HPP
