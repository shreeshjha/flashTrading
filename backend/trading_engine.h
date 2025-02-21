#ifndef TRADING_ENGINE_H
#define TRADING_ENGINE_H

#include <string>
#include <vector>
#include <tuple>

#ifdef __cplusplus
extern "C" {
#endif

// Fortran routine declarations (from advanced_order_book.f90)
void add_order(int id, const char* symbol, double price, int quantity, char side);
void cancel_order(const char* symbol, int id, int* status);
void modify_order(const char* symbol, int id, double new_price, int new_quantity, int* status);
void get_order_count(const char* symbol, int* count);
void get_order_book_snapshot(const char* symbol, double* out_prices, int* out_qtys, char* out_sides, int* out_count);
void get_trades(const char* symbol, double* out_prices, int* out_qtys, char* out_sides, int* out_tids, int* out_count);

#ifdef __cplusplus
}
#endif

// C++ wrappers
void cpp_add_order(int id, const std::string &symbol, double price, int quantity, char side);
int cpp_cancel_order(const std::string &symbol, int id);
int cpp_modify_order(const std::string &symbol, int id, double new_price, int new_quantity);
int cpp_get_order_count(const std::string &symbol);
std::vector<std::tuple<double,int,char>> cpp_get_order_book_snapshot(const std::string &symbol);

struct TradeData {
    int trade_id;
    double price;
    int quantity;
    char side;
};

std::vector<TradeData> cpp_get_trades(const std::string &symbol);

#endif // TRADING_ENGINE_H

