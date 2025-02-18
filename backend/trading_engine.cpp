#include "trading_engine.h"
#include <iostream>

extern "C" {
    void add_order(int id, double price, int quantity, char side);
    void get_order_count(int *count);
    void cancel_order(int id, int *status);
    void modify_order(int id, double new_price, int new_quantity, int *status);
}

void cpp_add_order(int id, double price, int quantity, char side) {
    add_order(id, price, quantity, side);
}

int cpp_get_order_count() {
    int count = 0;
    get_order_count(&count);
    return count;
}

int cpp_cancel_order(int id) {
    int status = 1;
    cancel_order(id, &status);
    return status;
}

int cpp_modify_order(int id, double new_price, int new_quantity) {
    int status = 1;
    modify_order(id, new_price, new_quantity, &status);
    return status;
}

// Optional test main (if needed, enable via a macro)
// #ifdef TEST_ENGINE
// int main() {
//     cpp_add_order(101, 99.5, 10, 'B');
//     cpp_add_order(102, 100.0, 20, 'S');
//     std::cout << "Order count: " << cpp_get_order_count() << std::endl;
//     int status = cpp_cancel_order(101);
//     std::cout << "Cancel status: " << status << std::endl;
//     status = cpp_modify_order(102, 101.5, 25);
//     std::cout << "Modify status: " << status << std::endl;
//     return 0;
// }
// #endif
