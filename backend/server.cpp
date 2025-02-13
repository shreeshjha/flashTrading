#include "crow_all.h"
#include <cstdlib>
#include <string> 

extern "C" {
    void add_order(int id, double price, int quantity, char side);
    void get_order_count(int *count);
}

// C++ wrappers as defined in trading_engine.cpp 

void cpp_add_order(int id, double price, int quantity, char side) {
    add_order(id, price, quantity, side);
}

int cpp_get_order_count() {
    int count = 0;
    get_order_count(&count);
    return count;
}

int main() {
    crow::SimpleApp app;
    CROW_ROUTE(app, "/add_order").methods("POST"_method)
        ([&](const crow::request& req) {
        auto json_body = crow::json::load(req.body);
        if(!json_body) 
            return crow::response(400, "Invalid JSON");

        int id = json["id"].i();
        double price = json_body["price"].d();
        int quantity = json_body["quantity"].i();
        std::string side_str = json_body["side"].s();
        char side = side_str.empty() ? 'B' : side_str[0];

        cpp_add_order(id, price, quantity, side);
        return crow::response(200, "Order added");
    });

    // Endpoint to get the current order count. 
    CROW_ROUTE(app, "/order_count")
        ([]() {
         int count = cpp_get_order_count();
         crow::json::wvalue result;
         result["order_count"] = count;
         return crow::response(result);
         });
    app.port(18080).multithreaded().run();
    return 0;
}
