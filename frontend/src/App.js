import React, { useState, useEffect } from 'react';

function App() {
  const [symbol, setSymbol] = useState("AAPL");
  const [orderCount, setOrderCount] = useState(0);
  const [orderBook, setOrderBook] = useState([]);
  const [trades, setTrades] = useState([]);
  const [wsMessage, setWsMessage] = useState("");

  // For add/cancel/modify forms
  const [addOrder, setAddOrder] = useState({ id: "", price: "", quantity: "", side: "B" });
  const [cancelOrder, setCancelOrder] = useState({ id: "" });
  const [modifyOrder, setModifyOrder] = useState({ id: "", new_price: "", new_quantity: "" });

  // Fetch order count
  const fetchOrderCount = async (sym) => {
    try {
      const res = await fetch(`http://localhost:18080/order_count?symbol=${sym}`);
      const data = await res.json();
      setOrderCount(data.order_count);
    } catch (err) {
      console.error("Error fetching order count:", err);
    }
  };

  // Fetch order book
  const fetchOrderBook = async (sym) => {
    try {
      const res = await fetch(`http://localhost:18080/order_book?symbol=${sym}`);
      const data = await res.json();
      setOrderBook(data.orders || []);
    } catch (err) {
      console.error("Error fetching order book:", err);
    }
  };

  // Fetch trades
  const fetchTrades = async (sym) => {
    try {
      const res = await fetch(`http://localhost:18080/trades?symbol=${sym}`);
      const data = await res.json();
      setTrades(data.trades || []);
    } catch (err) {
      console.error("Error fetching trades:", err);
    }
  };

  // On symbol change, refetch data
  useEffect(() => {
    fetchOrderCount(symbol);
    fetchOrderBook(symbol);
    fetchTrades(symbol);
  }, [symbol]);

  // Poll for data every few seconds
  useEffect(() => {
    const interval = setInterval(() => {
      fetchOrderCount(symbol);
      fetchOrderBook(symbol);
      fetchTrades(symbol);
    }, 5000);
    return () => clearInterval(interval);
  }, [symbol]);

  // WebSocket for live updates
  useEffect(() => {
    const ws = new WebSocket("ws://localhost:18080/ws");
    ws.onopen = () => {
      // send initial symbol
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

  // Add order
  const handleAddOrder = async () => {
    const payload = {
      symbol,
      id: parseInt(addOrder.id),
      price: parseFloat(addOrder.price),
      quantity: parseInt(addOrder.quantity),
      side: addOrder.side
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

  // Cancel order
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

  // Modify order
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

  return (
    <div style={{ padding: "2rem", maxWidth: "900px", margin: "0 auto", fontFamily: "Arial, sans-serif" }}>
      <h1>Trading Simulator Dashboard</h1>

      <label>Select Symbol: </label>
      <select value={symbol} onChange={e => setSymbol(e.target.value)}>
        <option value="AAPL">AAPL</option>
        <option value="MSFT">MSFT</option>
        {/* Add more symbols if desired */}
      </select>

      <p style={{ marginTop: "1rem" }}>Current Order Count: <strong>{orderCount}</strong></p>
      <p style={{ color: "#007700" }}>WebSocket Update: {wsMessage}</p>

      <hr />
      <h3>Add Order</h3>
      <input type="number" placeholder="Order ID"
        value={addOrder.id} onChange={e => setAddOrder({...addOrder, id: e.target.value})} />
      <input type="number" placeholder="Price"
        value={addOrder.price} onChange={e => setAddOrder({...addOrder, price: e.target.value})} />
      <input type="number" placeholder="Quantity"
        value={addOrder.quantity} onChange={e => setAddOrder({...addOrder, quantity: e.target.value})} />
      <select value={addOrder.side} onChange={e => setAddOrder({...addOrder, side: e.target.value})}>
        <option value="B">Buy</option>
        <option value="S">Sell</option>
      </select>
      <button onClick={handleAddOrder}>Add Order</button>

      <hr />
      <h3>Cancel Order</h3>
      <input type="number" placeholder="Order ID"
        value={cancelOrder.id} onChange={e => setCancelOrder({...cancelOrder, id: e.target.value})} />
      <button onClick={handleCancelOrder}>Cancel Order</button>

      <hr />
      <h3>Modify Order</h3>
      <input type="number" placeholder="Order ID"
        value={modifyOrder.id} onChange={e => setModifyOrder({...modifyOrder, id: e.target.value})} />
      <input type="number" placeholder="New Price"
        value={modifyOrder.new_price} onChange={e => setModifyOrder({...modifyOrder, new_price: e.target.value})} />
      <input type="number" placeholder="New Quantity"
        value={modifyOrder.new_quantity} onChange={e => setModifyOrder({...modifyOrder, new_quantity: e.target.value})} />
      <button onClick={handleModifyOrder}>Modify Order</button>

      <hr />
      <h2>Order Book for {symbol}</h2>
      <table border="1" cellPadding="5">
        <thead>
          <tr><th>Price</th><th>Quantity</th><th>Side</th></tr>
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

      <hr />
      <h2>Recent Trades for {symbol}</h2>
      <table border="1" cellPadding="5">
        <thead>
          <tr><th>Trade ID</th><th>Price</th><th>Quantity</th><th>Side</th></tr>
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
  );
}

export default App;

