#include "crow.h"
#include <cstdlib>
#include <string>

extern "C" {
    void cpp_add_order(int id, double price, int quantity, char side);
    int cpp_get_order_count();
}

// Define a custom middleware to handle CORS
struct CORSMiddleware {
    struct context {};

    // Called before the main handler
    void before_handle(crow::request& req, crow::response& res, context&) {
        // If this is an OPTIONS request, we can end early (Crow will still call after_handle).
        if (req.method == crow::HTTPMethod::Options) {
            res.code = 204;
            res.end();
        }
    }

    // Called after the main handler
    void after_handle(crow::request& req, crow::response& res, context&) {
        // Ensure these CORS headers are always present on every response
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "*");
    }
};

int main() {
    // Use the middleware in the Crow app.
    crow::App<CORSMiddleware> app;

    // Route to fetch the current order count
    CROW_ROUTE(app, "/order_count")
    .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options) {
            // We already set 204 in before_handle, but returning is safe.
            return crow::response(204);
        }
        int count = cpp_get_order_count();
        crow::json::wvalue result;
        result["order_count"] = count;
        return crow::response(result);
    });

    // Route to add a new order
    CROW_ROUTE(app, "/add_order")
    .methods(crow::HTTPMethod::Post, crow::HTTPMethod::Options)
    ([](const crow::request& req) {
        if(req.method == crow::HTTPMethod::Options) {
            return crow::response(204);
        }
        try {
            auto json_body = crow::json::load(req.body);
            if (!json_body) {
                return crow::response(400, "Invalid JSON");
            }
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

    // Start the server on port 18080, using multiple threads
    app.port(18080).multithreaded().run();
    return 0;
}
