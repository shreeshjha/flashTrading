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

  // WebSocket
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

  // Handlers
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

  // Chart data
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

