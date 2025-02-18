#ifndef TRADING_ENGINE_H
#define TRADING_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

// Declarations of the wrappers implemented in trading_engine.cpp.
void cpp_add_order(int id, double price, int quantity, char side);
int cpp_get_order_count();
int cpp_cancel_order(int id);
int cpp_modify_order(int id, double new_price, int new_quantity);

#ifdef __cplusplus
}
#endif

#endif // TRADING_ENGINE_H
