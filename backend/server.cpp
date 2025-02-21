#include "crow.h"
#include "trading_engine.h"
#include <cstdlib>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <mutex>

std::mutex logMutex;
void log_message(const std::string &msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << msg << std::endl;
}

void marketMakerTask(const std::string &symbol) {
    while (true) {
        int buyId = rand() % 10000 + 1000;
        double bidPrice = 100.0 + ((rand() % 100) / 10.0);
        int buyQty = (rand() % 50) + 1;
        cpp_add_order(buyId, symbol, bidPrice, buyQty, 'B');

        int sellId = rand() % 10000 + 20000;
        double askPrice = 100.0 + ((rand() % 100) / 10.0);
        int sellQty = (rand() % 50) + 1;
        cpp_add_order(sellId, symbol, askPrice, sellQty, 'S');

        log_message("MarketMaker posted orders for " + symbol);
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

struct CORSMiddleware {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context&) {
        if (req.method == crow::HTTPMethod::Options) {
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
    std::thread mmAAPL(marketMakerTask, "AAPL");
    mmAAPL.detach();
    std::thread mmMSFT(marketMakerTask, "MSFT");
    mmMSFT.detach();

    crow::App<CORSMiddleware> app;

    CROW_ROUTE(app, "/order_count")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req){
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
    
    CROW_ROUTE(app, "/add_order")
.methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
([](const crow::request& req) {
    if(req.method == crow::HTTPMethod::Options)
        return crow::response(204);

    try {
        // Log incoming request
        std::cout << "Received order request: " << req.body << std::endl;
        
        // Ensure content type is correct
        auto contentType = req.get_header_value("Content-Type");
        if (contentType.find("application/json") == std::string::npos) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Content-Type must be application/json";
            return crow::response(400, error);
        }

        // Parse JSON body
        auto order = crow::json::load(req.body);
        if (!order) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Invalid JSON format";
            return crow::response(400, error);
        }

        // Validate required fields
        if (!order.has("symbol") || !order.has("id") || 
            !order.has("price") || !order.has("quantity") || 
            !order.has("side")) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Missing required fields";
            return crow::response(400, error);
        }

        // Extract values
        std::string symbol = order["symbol"].s();
        int id = order["id"].i();
        double price = order["price"].d();
        int quantity = order["quantity"].i();
        std::string side_str = order["side"].s();

        // Validate values
        if (symbol.empty() || price <= 0 || quantity <= 0 || 
            (side_str != "B" && side_str != "S")) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Invalid field values";
            return crow::response(400, error);
        }

        char side = side_str[0];

        // Add the order
        cpp_add_order(id, symbol, price, quantity, side);

        // Return success response
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
    

    CROW_ROUTE(app, "/cancel_order")
    .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([](const crow::request& req){
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

    CROW_ROUTE(app, "/modify_order")
    .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([](const crow::request& req){
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

    CROW_ROUTE(app, "/order_book")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req){
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

    CROW_ROUTE(app, "/trades")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req){
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

    CROW_ROUTE(app, "/ws")
    .websocket()
    .onopen([](crow::websocket::connection& conn){
         conn.send_text("Connected to WebSocket. Please send a symbol.");
    })
    .onmessage([](crow::websocket::connection& conn, const std::string& data, bool){
         std::string symbol = data;
         std::thread([symbol, &conn](){
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
    .onclose([](crow::websocket::connection& conn, const std::string& reason){
         // Handle closure if needed.
    });

    app.port(18080).multithreaded().run();
    return 0;
}
