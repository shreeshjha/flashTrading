#include "crow.h"
#include "trading_engine.h"  // Include the declarations from trading_engine.h
#include <cstdlib>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>

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
    // Start a market maker thread.
    std::thread marketMakerThread([](){
        while (true) {
            int id = rand() % 10000 + 1000;
            double price = 100.0 + (rand() % 100) / 10.0;
            int quantity = (rand() % 50) + 1;
            char side = (rand() % 2 == 0) ? 'B' : 'S';
            cpp_add_order(id, price, quantity, side);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    marketMakerThread.detach();

    crow::App<CORSMiddleware> app;

    // Get order count endpoint.
    CROW_ROUTE(app, "/order_count")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if (req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        int count = cpp_get_order_count();
        crow::json::wvalue result;
        result["order_count"] = count;
        return crow::response(result);
    });

    // Add order endpoint.
    CROW_ROUTE(app, "/add_order")
    .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if (req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        try {
            auto json_body = crow::json::load(req.body);
            if (!json_body)
                return crow::response(400, "Invalid JSON");
            int id = json_body["id"].i();
            double price = json_body["price"].d();
            int quantity = json_body["quantity"].i();
            std::string side_str = json_body["side"].s();
            char side = side_str.empty() ? 'B' : side_str[0];
            cpp_add_order(id, price, quantity, side);
            crow::json::wvalue result;
            result["status"] = "success";
            result["message"] = "Order added successfully";
            return crow::response(result);
        } catch (const std::exception& e) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = e.what();
            return crow::response(500, error);
        }
    });

    // Cancel order endpoint.
    CROW_ROUTE(app, "/cancel_order")
    .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if (req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        try {
            auto json_body = crow::json::load(req.body);
            if (!json_body)
                return crow::response(400, "Invalid JSON");
            int id = json_body["id"].i();
            int status = cpp_cancel_order(id);
            crow::json::wvalue result;
            result["status"] = (status == 0) ? "success" : "error";
            result["message"] = (status == 0) ? "Order cancelled" : "Order not found";
            return crow::response(result);
        } catch (const std::exception& e) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = e.what();
            return crow::response(500, error);
        }
    });

    // Modify order endpoint.
    CROW_ROUTE(app, "/modify_order")
    .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if (req.method == crow::HTTPMethod::Options)
            return crow::response(204);
        try {
            auto json_body = crow::json::load(req.body);
            if (!json_body)
                return crow::response(400, "Invalid JSON");
            int id = json_body["id"].i();
            double new_price = json_body["new_price"].d();
            int new_quantity = json_body["new_quantity"].i();
            int status = cpp_modify_order(id, new_price, new_quantity);
            crow::json::wvalue result;
            result["status"] = (status == 0) ? "success" : "error";
            result["message"] = (status == 0) ? "Order modified" : "Order not found";
            return crow::response(result);
        } catch (const std::exception& e) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = e.what();
            return crow::response(500, error);
        }
    });

    app.port(18080).multithreaded().run();
    return 0;
}
