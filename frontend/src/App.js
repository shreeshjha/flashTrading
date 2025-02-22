// App.js
import React, { useState, useEffect } from 'react';
import { Line } from 'react-chartjs-2';
import 'chart.js/auto';
import 'bootstrap/dist/css/bootstrap.min.css';

function App() {
  const [symbol, setSymbol] = useState("AAPL");
  const [orderCount, setOrderCount] = useState(0);
  const [orderBook, setOrderBook] = useState([]);
  const [trades, setTrades] = useState([]);
  const [riskMetrics, setRiskMetrics] = useState(0);
  const [wsMessage, setWsMessage] = useState("");

  const [addOrder, setAddOrder] = useState({ id: "", price: "", quantity: "", side: "B", order_type: "0" });
  const [cancelOrder, setCancelOrder] = useState({ id: "" });
  const [modifyOrder, setModifyOrder] = useState({ id: "", new_price: "", new_quantity: "" });

  // Benchmark state
  const [benchmarkResult, setBenchmarkResult] = useState(null);
  const [benchmarkAdvancedResult, setBenchmarkAdvancedResult] = useState(null);
  const [isBenchmarking, setIsBenchmarking] = useState(false);

  // Fetch functions
  const fetchOrderCount = async (sym) => {
    try {
      const res = await fetch(`http://localhost:18080/order_count?symbol=${sym}`);
      const data = await res.json();
      setOrderCount(data.order_count);
    } catch (err) {
      console.error("Error fetching order count:", err);
    }
  };

  const fetchOrderBook = async (sym) => {
    try {
      const res = await fetch(`http://localhost:18080/order_book?symbol=${sym}`);
      const data = await res.json();
      setOrderBook(data.orders || []);
    } catch (err) {
      console.error("Error fetching order book:", err);
    }
  };

  const fetchTrades = async (sym) => {
    try {
      const res = await fetch(`http://localhost:18080/trades?symbol=${sym}`);
      const data = await res.json();
      setTrades(data.trades || []);
    } catch (err) {
      console.error("Error fetching trades:", err);
    }
  };

  const fetchRiskMetrics = async (sym) => {
    try {
      const res = await fetch(`http://localhost:18080/risk_metrics?symbol=${sym}`);
      const data = await res.json();
      setRiskMetrics(data.total_quantity);
    } catch (err) {
      console.error("Error fetching risk metrics:", err);
    }
  };

  // Initial & interval data fetch
  useEffect(() => {
    fetchOrderCount(symbol);
    fetchOrderBook(symbol);
    fetchTrades(symbol);
    fetchRiskMetrics(symbol);
  }, [symbol]);

  useEffect(() => {
    const interval = setInterval(() => {
      fetchOrderCount(symbol);
      fetchOrderBook(symbol);
      fetchTrades(symbol);
      fetchRiskMetrics(symbol);
    }, 5000);
    return () => clearInterval(interval);
  }, [symbol]);

  // WebSocket for live updates
  useEffect(() => {
    const ws = new WebSocket("ws://localhost:18080/ws");
    ws.onopen = () => {
      ws.send(symbol);
      console.log("WebSocket connected");
    };
    ws.onmessage = (e) => {
      setWsMessage(e.data);
    };
    ws.onerror = (e) => {
      console.error("WebSocket error:", e);
    };
    ws.onclose = () => {
      console.log("WebSocket closed");
    };
    return () => ws.close();
  }, [symbol]);

  // Order Handlers
  const handleAddOrder = async () => {
    const payload = {
      symbol,
      id: parseInt(addOrder.id),
      price: parseFloat(addOrder.price),
      quantity: parseInt(addOrder.quantity),
      side: addOrder.side,
      order_type: parseInt(addOrder.order_type)
    };
    try {
      await fetch("http://localhost:18080/add_order", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });
    } catch (err) {
      console.error("Add order failed:", err);
    }
  };

  const handleCancelOrder = async () => {
    const payload = {
      symbol,
      id: parseInt(cancelOrder.id)
    };
    try {
      await fetch("http://localhost:18080/cancel_order", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });
    } catch (err) {
      console.error("Cancel order failed:", err);
    }
  };

  const handleModifyOrder = async () => {
    const payload = {
      symbol,
      id: parseInt(modifyOrder.id),
      new_price: parseFloat(modifyOrder.new_price),
      new_quantity: parseInt(modifyOrder.new_quantity)
    };
    try {
      await fetch("http://localhost:18080/modify_order", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });
    } catch (err) {
      console.error("Modify order failed:", err);
    }
  };

  // Benchmark Handlers
  const handleBenchmark = async () => {
    setIsBenchmarking(true);
    try {
      const res = await fetch(`http://localhost:18080/benchmark?n=1000&symbol=${symbol}`);
      const data = await res.json();
      setBenchmarkResult(data);
    } catch (err) {
      console.error("Error running benchmark:", err);
    }
    setIsBenchmarking(false);
  };

  const handleBenchmarkAdvanced = async () => {
    setIsBenchmarking(true);
    try {
      const res = await fetch(`http://localhost:18080/benchmark_advanced?n=1000&c=4&symbol=${symbol}`);
      const data = await res.json();
      setBenchmarkAdvancedResult(data);
    } catch (err) {
      console.error("Error running advanced benchmark:", err);
    }
    setIsBenchmarking(false);
  };

  // Chart data for Order Book
  const orderBookData = {
    labels: orderBook.map((_, idx) => idx + 1),
    datasets: [
      {
        label: "Price",
        data: orderBook.map(o => o.price),
        borderColor: "blue",
        fill: false,
        tension: 0.1
      }
    ]
  };

  return (
    <>
      {/* Navbar */}
      <nav className="navbar navbar-expand-lg navbar-dark bg-dark mb-4">
        <div className="container-fluid">
          <a className="navbar-brand" href="#home">Trading Simulator</a>
        </div>
      </nav>

      {/* Main container */}
      <div className="container">
        {/* Symbol Selection & Stats */}
        <div className="row mb-4">
          <div className="col-md-4">
            <label className="form-label fw-bold">Select Symbol:</label>
            <select
              className="form-select"
              value={symbol}
              onChange={e => setSymbol(e.target.value)}
            >
              <option value="AAPL">AAPL</option>
              <option value="MSFT">MSFT</option>
            </select>
          </div>
          <div className="col-md-8">
            <p className="mb-1 mt-4">
              <strong>Current Order Count:</strong> {orderCount}
            </p>
            <p className="mb-1">
              <strong>Risk Metric (Total Quantity):</strong> {riskMetrics}
            </p>
            <p className="mb-1 text-success">
              <strong>WebSocket Update:</strong> {wsMessage}
            </p>
          </div>
        </div>

        {/* Orders Section */}
        <div className="row">
          {/* Add Order */}
          <div className="col-md-4 mb-4">
            <div className="card">
              <div className="card-header">Add Order</div>
              <div className="card-body">
                <div className="mb-3">
                  <label className="form-label">Order ID</label>
                  <input
                    type="number"
                    className="form-control"
                    placeholder="Order ID"
                    value={addOrder.id}
                    onChange={e => setAddOrder({...addOrder, id: e.target.value})}
                  />
                </div>
                <div className="mb-3">
                  <label className="form-label">Price</label>
                  <input
                    type="number"
                    className="form-control"
                    placeholder="Price"
                    value={addOrder.price}
                    onChange={e => setAddOrder({...addOrder, price: e.target.value})}
                  />
                </div>
                <div className="mb-3">
                  <label className="form-label">Quantity</label>
                  <input
                    type="number"
                    className="form-control"
                    placeholder="Quantity"
                    value={addOrder.quantity}
                    onChange={e => setAddOrder({...addOrder, quantity: e.target.value})}
                  />
                </div>
                <div className="mb-3">
                  <label className="form-label">Side</label>
                  <select
                    className="form-select"
                    value={addOrder.side}
                    onChange={e => setAddOrder({...addOrder, side: e.target.value})}
                  >
                    <option value="B">Buy</option>
                    <option value="S">Sell</option>
                  </select>
                </div>
                <div className="mb-3">
                  <label className="form-label">Order Type</label>
                  <select
                    className="form-select"
                    value={addOrder.order_type}
                    onChange={e => setAddOrder({...addOrder, order_type: e.target.value})}
                  >
                    <option value="0">Limit</option>
                    <option value="1">Market</option>
                  </select>
                </div>
                <button className="btn btn-primary w-100" onClick={handleAddOrder}>
                  Add Order
                </button>
              </div>
            </div>
          </div>

          {/* Cancel Order */}
          <div className="col-md-4 mb-4">
            <div className="card">
              <div className="card-header">Cancel Order</div>
              <div className="card-body">
                <div className="mb-3">
                  <label className="form-label">Order ID</label>
                  <input
                    type="number"
                    className="form-control"
                    placeholder="Order ID"
                    value={cancelOrder.id}
                    onChange={e => setCancelOrder({...cancelOrder, id: e.target.value})}
                  />
                </div>
                <button className="btn btn-warning w-100" onClick={handleCancelOrder}>
                  Cancel Order
                </button>
              </div>
            </div>
          </div>

          {/* Modify Order */}
          <div className="col-md-4 mb-4">
            <div className="card">
              <div className="card-header">Modify Order</div>
              <div className="card-body">
                <div className="mb-3">
                  <label className="form-label">Order ID</label>
                  <input
                    type="number"
                    className="form-control"
                    placeholder="Order ID"
                    value={modifyOrder.id}
                    onChange={e => setModifyOrder({...modifyOrder, id: e.target.value})}
                  />
                </div>
                <div className="mb-3">
                  <label className="form-label">New Price</label>
                  <input
                    type="number"
                    className="form-control"
                    placeholder="New Price"
                    value={modifyOrder.new_price}
                    onChange={e => setModifyOrder({...modifyOrder, new_price: e.target.value})}
                  />
                </div>
                <div className="mb-3">
                  <label className="form-label">New Quantity</label>
                  <input
                    type="number"
                    className="form-control"
                    placeholder="New Quantity"
                    value={modifyOrder.new_quantity}
                    onChange={e => setModifyOrder({...modifyOrder, new_quantity: e.target.value})}
                  />
                </div>
                <button className="btn btn-info w-100" onClick={handleModifyOrder}>
                  Modify Order
                </button>
              </div>
            </div>
          </div>
        </div>

        {/* Benchmark Section */}
        <div className="row mb-5">
          <div className="col-md-6">
            <button
              className="btn btn-primary w-100 mb-2"
              onClick={handleBenchmark}
              disabled={isBenchmarking}
            >
              {isBenchmarking ? "Running Single-Thread Benchmark..." : "Run Single-Thread Benchmark"}
            </button>
          </div>
          <div className="col-md-6">
            <button
              className="btn btn-success w-100 mb-2"
              onClick={handleBenchmarkAdvanced}
              disabled={isBenchmarking}
            >
              {isBenchmarking ? "Running Multi-Thread Benchmark..." : "Run Multi-Thread Benchmark"}
            </button>
          </div>
          {benchmarkResult && (
            <div className="col-md-12">
              <div className="card mt-3">
                <div className="card-header bg-primary text-white">
                  Single-Threaded Benchmark Result
                </div>
                <div className="card-body">
                  <p><strong>Symbol:</strong> {benchmarkResult.symbol}</p>
                  <p><strong>Orders Placed:</strong> {benchmarkResult.orders_placed}</p>
                  <p><strong>Total Time:</strong> {benchmarkResult.time_ms} ms</p>
                  <p>
                    <strong>Throughput:</strong>{" "}
                    {((benchmarkResult.orders_placed / benchmarkResult.time_ms) * 1000).toFixed(2)} orders/sec
                  </p>
                </div>
              </div>
            </div>
          )}
          {benchmarkAdvancedResult && (
            <div className="col-md-12">
              <div className="card mt-3">
                <div className="card-header bg-success text-white">
                  Multi-Threaded Benchmark Result
                </div>
                <div className="card-body">
                  <p><strong>Symbol:</strong> {benchmarkAdvancedResult.symbol}</p>
                  <p><strong>Threads:</strong> {benchmarkAdvancedResult.threads}</p>
                  <p><strong>Orders Per Thread:</strong> {benchmarkAdvancedResult.orders_per_thread}</p>
                  <p><strong>Total Orders:</strong> {benchmarkAdvancedResult.total_orders}</p>
                  <p><strong>Total Time:</strong> {benchmarkAdvancedResult.time_ms} ms</p>
                  <p>
                    <strong>Throughput:</strong> {benchmarkAdvancedResult.orders_per_sec.toFixed(2)} orders/sec
                  </p>
                  <p>
                    <strong>Avg Time Per Order:</strong> {benchmarkAdvancedResult.avg_time_per_order_ms.toFixed(3)} ms
                  </p>
                </div>
              </div>
            </div>
          )}
        </div>

        {/* Order Book & Chart */}
        <div className="row mb-5">
          <div className="col-md-12">
            <h4 className="mb-3">Order Book for {symbol}</h4>
            <div className="table-responsive">
              <table className="table table-bordered table-striped">
                <thead className="table-light">
                  <tr>
                    <th>Price</th>
                    <th>Quantity</th>
                    <th>Side</th>
                  </tr>
                </thead>
                <tbody>
                  {orderBook.map((o, idx) => (
                    <tr key={idx}>
                      <td>{o.price}</td>
                      <td>{o.quantity}</td>
                      <td>{o.side}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
            <div style={{ height: "400px" }}>
              <Line data={orderBookData} />
            </div>
          </div>
        </div>

        {/* Trades */}
        <div className="row mb-5">
          <div className="col-md-12">
            <h4 className="mb-3">Recent Trades for {symbol}</h4>
            <div className="table-responsive">
              <table className="table table-bordered table-striped">
                <thead className="table-light">
                  <tr>
                    <th>Trade ID</th>
                    <th>Price</th>
                    <th>Quantity</th>
                    <th>Side</th>
                  </tr>
                </thead>
                <tbody>
                  {trades.map((t, idx) => (
                    <tr key={idx}>
                      <td>{t.trade_id}</td>
                      <td>{t.price}</td>
                      <td>{t.quantity}</td>
                      <td>{t.side}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        </div>

      </div>
    </>
  );
}

export default App;

