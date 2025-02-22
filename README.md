# flashTrading - Trading Simulator

A lightweight trading simulator written in Fortran and C++, with a React frontend for visualization. This project demonstrates a simple order-matching engine, a continuously running feed of random orders, FIX integration, Lua scripting, and real-time REST/WebSocket endpoints via Crow. It also includes benchmarking endpoints to measure throughput and latency.

## Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Prerequisites](#prerequisites)
- [Build and Run](#build-and-run)
  - [1. Compile Fortran & C++ Core](#1-compile-fortran--c-core)
  - [2. Start the Crow Server](#2-start-the-crow-server)
  - [3. (Optional) Run the Feed Generator](#3-optional-run-the-feed-generator)
  - [4. Launch the React Frontend](#4-launch-the-react-frontend)
- [Usage](#usage)
  - [REST Endpoints](#rest-endpoints)
  - [WebSocket Endpoint](#websocket-endpoint)
- [Benchmarking](#benchmarking)
  - [Single-Thread Benchmark](#single-thread-benchmark)
  - [Multi-Thread Benchmark](#multi-thread-benchmark)
  - [Sample Benchmark Results](#sample-benchmark-results)
- [FIX Integration](#fix-integration)
- [Lua Integration](#lua-integration)

## Features

- **Order Book** in Fortran (supports limit/market orders).
- **C++ Wrapper** to expose Fortran subroutines for adding, canceling, and modifying orders.
- **Crow HTTP/REST Server** for external interaction:
  - `/add_order`, `/cancel_order`, `/modify_order`
  - `/order_book`, `/trades`, `/order_count`, `/risk_metrics`
  - WebSocket endpoint for live order count updates.
- **Continuous Market Maker** feed generating random buy/sell orders.
- **Benchmarking** endpoints to measure performance.
- **React Frontend** for a real-time dashboard with charting and management forms.

## Architecture

```scss
 ┌──────────────────┐     ┌─────────────────────┐
 │   advanced_      │     │  feed.cpp (C++)     │
 │ order_book.f90   │     │(Random order poster)│
 └──────┬───────────┘     └─────────┬───────────┘
        │                           │
        │ (Fortran)                 │
        │                           │
 ┌──────▼───────────┐     ┌─────────▼───────────┐
 │ trading_engine.cpp│     │ fix_integration.cpp │
 │  (C++ Wrappers)   │     │   (FIX support)     │
 └──────┬───────────┘     └─────────┬───────────┘
        │                           │
        │                           │
 ┌──────▼────────────┐    ┌─────────▼───────────┐
 │   server.cpp       │    │ lua_integration.cpp │
 │  (Crow REST/WS)    │    │  (Lua scripting)    │
 └────────┬───────────┘    └─────────────────────┘
          │
          │ (HTTP/JSON & WS)
          ▼
 ┌─────────────────────────┐
 │        React App        │
 │  (charts & order forms) │
 └─────────────────────────┘
```


## Prerequisites

- C++17 (or higher) compiler
- Fortran compiler (e.g., `gfortran`)
- CMake (optional, but recommended) or a build system of your choice
- Node.js (v14+ or v16+ recommended) and npm or yarn for the React frontend
- curl library for `feed.cpp`
- Crow library (the code includes Crow headers; you can build from source or link them directly)

## Build and Run

### 1. Compile Fortran & C++ Core

1. Compile Fortran module (`advanced_order_book.f90`) into an object or static library:
```bash
gfortran -c advanced_order_book.f90 -o advanced_order_book.o
```

2. Compile C++ engine (`trading_engine.cpp` etc.) and link against the Fortran object:
```bash
g++ -std=c++17 -c trading_engine.cpp -o trading_engine.o
g++ -std=c++17 -c server.cpp -o server.o
# Link them together, including the Fortran runtime
g++ server.o trading_engine.o advanced_order_book.o -o simulator -lgfortran -lpthread
```
Adjust libraries (`-lgfortran`, `-lpthread`, etc.) as needed for your environment.

### 2. Start the Crow Server: 

Once built, you can run:
```bash
./simulator
```
This starts the HTTP server on port 18080 (default). You’ll see logs indicating market maker threads are posting orders.

### 3. (Optional) Run the Feed Generator:
If you also want the feed to run in parallel (posting random orders to the server):
```bash
g++ feed.cpp -o feed -lcurl -lpthread -std=c++17
./feed
```
It continuously sends orders to http://127.0.0.1:18080/add_order every 3 seconds.

### 4. Launch the React Frontend:
1. Install dependencies:
```bash
cd frontend
npm install
```

2. Start the development server:
```bash
npm start
```

3. Open your browser at `http://localhost:3000`. You should see the trading dashboard.

## Usage
### REST Endpoints
- GET `/order_count?symbol=XYZ` : Returns the current number of orders for symbol XYZ.
- POST `/add_order` : Accepts JSON body with symbol, id, price, quantity, side, order_type.
- POST `/cancel_order` : Accepts JSON body with symbol, id.
- POST `/modify_order` :  Accepts JSON body with symbol, id, new_price, new_quantity.
- GET `/order_book?symbol=XYZ` :  Returns the current order book for symbol XYZ.
- GET `/trades?symbol=XYZ` :  Returns recent trades for symbol XYZ.
- GET `/risk_metrics?symbol=XYZ` :  Returns a simple “total quantity” metric for symbol XYZ.

### WebSocket Endpoint
- `ws://localhost:18080/ws` : Send a symbol string (e.g., "AAPL") to start receiving live order count updates.


## Benchmarking
### Single-Thread Benchmark
- Route: GET `/benchmark`
- Params:
    - n: number of orders to place (default 100)
    - symbol: which symbol to use (default “AAPL”)
- Example:
```bash
curl -i "http://localhost:18080/benchmark?n=1000&symbol=AAPL"
```
Response (JSON):
```text
{
  "symbol": "AAPL",
  "orders_placed": 1000,
  "time_ms": 22
}
```

### Multi-Thread Benchmark
- Route: GET `/benchmark_advanced`
- Params:
    - n: orders per thread (default 100)
    - c: number of threads (default 1)
    - symbol: symbol to use (default “AAPL”)
- Example:
```bash
curl -i "http://localhost:18080/benchmark_advanced?n=1000&c=4&symbol=AAPL"
```
Response (JSON):
```text
{
  "symbol": "AAPL",
  "threads": 4,
  "orders_per_thread": 1000,
  "total_orders": 4000,
  "time_ms": 62,
  "orders_per_sec": 64516.13,
  "avg_time_per_order_ms": 0.015
}
```

### Sample Benchmark Results
In our test environment, we observed the following:
```text 
    Single-Threaded:
        1000 orders took ~22 ms
        Throughput ~45k orders/sec
    Multi-Threaded (4 threads, total 4000 orders):
        62 ms total
        ~64k orders/sec throughput
        Average time per order ~0.015 ms
```

## FIX Integration
- `fix_integration.cpp` uses the `QuickFIX` library to listen for FIX messages (NewOrderSingle).
- Upon receiving a NewOrderSingle, it calls the C++ wrapper cpp_add_order with the extracted fields.
  
To run:
- Install QuickFIX.
- Build fix_integration.cpp with QuickFIX libraries.
- Run it with your FIX config file:
```bash
./simulator_fix path/to/fix.cfg
```
- The simulator will add orders to the Fortran engine when it receives `NewOrderSingle` messages.

## Lua Integration
- lua_integration.cpp uses the Lua C API to load a script (backtest_script.lua) and register the function add_order.
- You can call add_order(id, symbol, price, quantity, side, [order_type]) directly from Lua.

To run:
- Install Lua 5.3 or 5.4.
- Build lua_integration.cpp with Lua libraries.
- Create a backtest_script.lua with calls to add_order.
- Run:
```bash
./simulator_lua
```

