// server.cpp
#include "crow.h"
#include "trading_engine.h"
#include <cstdlib>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <mutex>
#include <vector>

std::mutex logMutex;
void log_message(const std::string &msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << msg << std::endl;
}

void placeRandomOrders(int n, const std::string &symbol) {
    for (int i = 0; i < n; i++) {
        int orderId = rand() % 100000 + 30000;
        double price = 100.0 + (rand() % 50);
        int quantity = (rand() % 10) + 1;
        char side = (rand() % 2) == 0 ? 'B' : 'S';
        int orderType = (rand() % 2) == 0 ? 0 : 1; // 0 = limit, 1 = market
        cpp_add_order(orderId, symbol, price, quantity, side, orderType);
    }
}

void marketMakerTask(const std::string &symbol) {
    while (true) {
        int buyId = rand() % 10000 + 1000;
        double bidPrice = 100.0 + ((rand() % 100) / 10.0);
        int buyQty = (rand() % 50) + 1;
        cpp_add_order(buyId, symbol, bidPrice, buyQty, 'B', 0);

        int sellId = rand() % 10000 + 20000;
        double askPrice = 100.0 + ((rand() % 100) / 10.0);
        int sellQty = (rand() % 50) + 1;
        cpp_add_order(sellId, symbol, askPrice, sellQty, 'S', 0);

        log_message("MarketMaker posted orders for " + symbol);
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

struct CORSMiddleware {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context&) {
        if(req.method == crow::HTTPMethod::Options) {
            res.code = 204;
            res.end();
        }
    }
    void after_handle(crow::request& req, crow::response& res, context&) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "*");
    }
};

int main() {
    // Start market-maker threads for AAPL and MSFT
    std::thread mmAAPL(marketMakerTask, "AAPL");
    mmAAPL.detach();
    std::thread mmMSFT(marketMakerTask, "MSFT");
    mmMSFT.detach();

    crow::App<CORSMiddleware> app;

    // GET /order_count
    CROW_ROUTE(app, "/order_count")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        auto sym = req.url_params.get("symbol");
        if(!sym)
            return crow::response(400, "Missing symbol param");
        int count = cpp_get_order_count(sym);
        crow::json::wvalue result;
        result["order_count"] = count;
        return crow::response(result);
    });

    // POST /add_order
    CROW_ROUTE(app, "/add_order")
    .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        try {
            std::cout << "Received order request: " << req.body << std::endl;
            auto contentType = req.get_header_value("Content-Type");
            if(contentType.find("application/json") == std::string::npos) {
                crow::json::wvalue error;
                error["status"] = "error";
                error["message"] = "Content-Type must be application/json";
                return crow::response(400, error);
            }
            auto order = crow::json::load(req.body);
            if(!order) {
                crow::json::wvalue error;
                error["status"] = "error";
                error["message"] = "Invalid JSON format";
                return crow::response(400, error);
            }
            if(!order.has("symbol") || !order.has("id") ||
               !order.has("price") || !order.has("quantity") ||
               !order.has("side") || !order.has("order_type")) {
                crow::json::wvalue error;
                error["status"] = "error";
                error["message"] = "Missing required fields";
                return crow::response(400, error);
            }
            std::string symbol = order["symbol"].s();
            int id = order["id"].i();
            double price = order["price"].d();
            int quantity = order["quantity"].i();
            std::string side_str = order["side"].s();
            int order_type = order["order_type"].i();
            if(symbol.empty() || price <= 0 || quantity <= 0 ||
               (side_str != "B" && side_str != "S")) {
                crow::json::wvalue error;
                error["status"] = "error";
                error["message"] = "Invalid field values";
                return crow::response(400, error);
            }
            char side = side_str[0];
            cpp_add_order(id, symbol, price, quantity, side, order_type);
            crow::json::wvalue response;
            response["status"] = "success";
            response["order_id"] = id;
            return crow::response(response);
        } catch (const std::exception& e) {
            std::cerr << "Exception in add_order: " << e.what() << std::endl;
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = std::string("Server error: ") + e.what();
            return crow::response(500, error);
        }
    });

    // POST /cancel_order
    CROW_ROUTE(app, "/cancel_order")
    .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        try {
            auto body = crow::json::load(req.body);
            if(!body)
                return crow::response(400, "Invalid JSON");
            std::string symbol = body["symbol"].s();
            int id = body["id"].i();
            int status = cpp_cancel_order(symbol, id);
            crow::json::wvalue resp;
            resp["status"] = (status == 0) ? "success" : "not_found";
            return crow::response(resp);
        } catch (std::exception &e) {
            return crow::response(500, e.what());
        }
    });

    // POST /modify_order
    CROW_ROUTE(app, "/modify_order")
    .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        try {
            auto body = crow::json::load(req.body);
            if(!body)
                return crow::response(400, "Invalid JSON");
            std::string symbol = body["symbol"].s();
            int id = body["id"].i();
            double new_price = body["new_price"].d();
            int new_quantity = body["new_quantity"].i();
            int status = cpp_modify_order(symbol, id, new_price, new_quantity);
            crow::json::wvalue resp;
            resp["status"] = (status == 0) ? "success" : "not_found";
            return crow::response(resp);
        } catch (std::exception &e) {
            return crow::response(500, e.what());
        }
    });

    // GET /order_book
    CROW_ROUTE(app, "/order_book")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        auto sym = req.url_params.get("symbol");
        if(!sym)
            return crow::response(400, "Missing symbol param");
        auto snapshot = cpp_get_order_book_snapshot(sym);
        crow::json::wvalue result;
        crow::json::wvalue::list orders;
        for (auto &t : snapshot) {
            crow::json::wvalue item;
            item["price"] = std::get<0>(t);
            item["quantity"] = std::get<1>(t);
            item["side"] = std::string(1, std::get<2>(t));
            orders.push_back(std::move(item));
        }
        result["orders"] = std::move(orders);
        return crow::response(result);
    });

    // GET /trades
    CROW_ROUTE(app, "/trades")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        auto sym = req.url_params.get("symbol");
        if(!sym)
            return crow::response(400, "Missing symbol param");
        auto trades = cpp_get_trades(sym);
        crow::json::wvalue result;
        crow::json::wvalue::list arr;
        for (auto &td : trades) {
            crow::json::wvalue item;
            item["trade_id"] = td.trade_id;
            item["price"] = td.price;
            item["quantity"] = td.quantity;
            item["side"] = std::string(1, td.side);
            arr.push_back(std::move(item));
        }
        result["trades"] = std::move(arr);
        return crow::response(result);
    });

    // GET /risk_metrics
    CROW_ROUTE(app, "/risk_metrics")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        auto sym = req.url_params.get("symbol");
        if(!sym)
            return crow::response(400, "Missing symbol param");
        int total_qty = cpp_get_risk_metrics(sym);
        crow::json::wvalue result;
        result["total_quantity"] = total_qty;
        return crow::response(result);
    });

    // GET /benchmark - simple single-thread benchmark
    CROW_ROUTE(app, "/benchmark")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        int n = 100;
        auto nParam = req.url_params.get("n");
        if(nParam) {
            n = std::atoi(nParam);
            if(n <= 0) n = 100;
        }
        std::string symbol = "AAPL";
        auto sym = req.url_params.get("symbol");
        if(sym) symbol = sym;
        auto start = std::chrono::high_resolution_clock::now();
        placeRandomOrders(n, symbol);
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        crow::json::wvalue result;
        result["symbol"] = symbol;
        result["orders_placed"] = n;
        result["time_ms"] = (long)elapsed;
        return crow::response(result);
    });

    // GET /benchmark_advanced - concurrent benchmark
    // Usage: /benchmark_advanced?n=1000&c=4&symbol=MSFT
    CROW_ROUTE(app, "/benchmark_advanced")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        int n = 100;
        int c = 1;
        auto nParam = req.url_params.get("n");
        if(nParam) {
            n = std::atoi(nParam);
            if(n <= 0) n = 100;
        }
        auto cParam = req.url_params.get("c");
        if(cParam) {
            c = std::atoi(cParam);
            if(c <= 0) c = 1;
        }
        std::string symbol = "AAPL";
        auto sym = req.url_params.get("symbol");
        if(sym) symbol = sym;
        auto start = std::chrono::high_resolution_clock::now();
        {
            std::vector<std::thread> threads;
            threads.reserve(c);
            for(int i = 0; i < c; i++) {
                threads.emplace_back(placeRandomOrders, n, symbol);
            }
            for(auto &t : threads) {
                t.join();
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        long total_orders = (long)n * (long)c;
        double elapsed_sec = (double)elapsed_ms / 1000.0;
        double orders_per_sec = elapsed_sec > 0 ? (double)total_orders / elapsed_sec : 0.0;
        double avg_time_per_order_ms = elapsed_ms / (double)total_orders;
        crow::json::wvalue result;
        result["symbol"] = symbol;
        result["threads"] = c;
        result["orders_per_thread"] = n;
        result["total_orders"] = total_orders;
        result["time_ms"] = (long)elapsed_ms;
        result["orders_per_sec"] = orders_per_sec;
        result["avg_time_per_order_ms"] = avg_time_per_order_ms;
        return crow::response(result);
    });

    // WebSocket endpoint for live updates
    CROW_ROUTE(app, "/ws")
    .websocket()
    .onopen([](crow::websocket::connection& conn) {
         conn.send_text("Connected to WebSocket. Please send a symbol.");
    })
    .onmessage([](crow::websocket::connection& conn, const std::string& data, bool) {
         std::string symbol = data;
         std::thread([symbol, &conn]() {
             while (true) {
                 int count = cpp_get_order_count(symbol);
                 try {
                     conn.send_text("Live " + symbol + " Order Count: " + std::to_string(count));
                 } catch (...) {
                     break;
                 }
                 std::this_thread::sleep_for(std::chrono::seconds(1));
             }
         }).detach();
    })
    .onclose([](crow::websocket::connection& conn, const std::string& reason) {
         // Handle WebSocket closure if needed.
    });

    app.port(18080).multithreaded().run();
    return 0;
}

