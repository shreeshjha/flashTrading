#include <iostream>

extern "C" {
    // Fortran routines defined in order_book.f90
    void add_order(int id, double price, int quantity, char side);
    void get_order_count(int *count);

    // Wrapper function to add an order
    void cpp_add_order(int id, double price, int quantity, char side) {
        add_order(id, price, quantity, side);
    }

    // Wrapper function to get the order count
    int cpp_get_order_count() {
        int count = 0;
        get_order_count(&count);
        return count;
    }
}

// Optional: A test main for debugging the trading engine separately
#ifdef TEST_ENGINE
int main() {
    cpp_add_order(101, 99.5, 10, 'B');
    cpp_add_order(102, 100.0, 20, 'S');
    std::cout << "Order count: " << cpp_get_order_count() << std::endl;
    return 0;
}
#endif
