#include "trading_engine.h"
#include <cstring>
#include <algorithm>
#include <mutex>

// Global mutex to protect Fortran calls.
static std::mutex fortranMutex;

// Helper: Convert std::string to an 8-character C string (padded with spaces)
static void string_to_c8(const std::string &s, char sym[9]) {
    std::memset(sym, ' ', 8);
    sym[8] = '\0';
    std::strncpy(sym, s.c_str(), std::min((size_t)8, s.size()));
}

void cpp_add_order(int id, const std::string &symbol, double price, int quantity, char side) {
    std::lock_guard<std::mutex> lock(fortranMutex);
    char sym[9];
    string_to_c8(symbol, sym);
    add_order(id, sym, price, quantity, side);
}

int cpp_cancel_order(const std::string &symbol, int id) {
    std::lock_guard<std::mutex> lock(fortranMutex);
    int status = 1;
    char sym[9];
    string_to_c8(symbol, sym);
    cancel_order(sym, id, &status);
    return status;
}

int cpp_modify_order(const std::string &symbol, int id, double new_price, int new_quantity) {
    std::lock_guard<std::mutex> lock(fortranMutex);
    int status = 1;
    char sym[9];
    string_to_c8(symbol, sym);
    modify_order(sym, id, new_price, new_quantity, &status);
    return status;
}

int cpp_get_order_count(const std::string &symbol) {
    std::lock_guard<std::mutex> lock(fortranMutex);
    int count = 0;
    char sym[9];
    string_to_c8(symbol, sym);
    get_order_count(sym, &count);
    return count;
}

std::vector<std::tuple<double,int,char>> cpp_get_order_book_snapshot(const std::string &symbol) {
    std::lock_guard<std::mutex> lock(fortranMutex);
    double prices[100];
    int qtys[100];
    char sides[100];
    int count = 0;
    char sym[9];
    string_to_c8(symbol, sym);
    get_order_book_snapshot(sym, prices, qtys, sides, &count);
    std::vector<std::tuple<double,int,char>> result;
    for (int i = 0; i < count; i++) {
        result.push_back(std::make_tuple(prices[i], qtys[i], sides[i]));
    }
    return result;
}

std::vector<TradeData> cpp_get_trades(const std::string &symbol) {
    std::lock_guard<std::mutex> lock(fortranMutex);
    double prices[1000];
    int qtys[1000], tids[1000];
    char sides[1000];
    int count = 0;
    char sym[9];
    string_to_c8(symbol, sym);
    get_trades(sym, prices, qtys, sides, tids, &count);
    std::vector<TradeData> result;
    for (int i = 0; i < count; i++) {
        TradeData t;
        t.trade_id = tids[i];
        t.price = prices[i];
        t.quantity = qtys[i];
        t.side = sides[i];
        result.push_back(t);
    }
    return result;
}
