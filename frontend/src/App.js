import React, { useState, useEffect } from 'react';

function App() {
  const [orderCount, setOrderCount] = useState(0);
  const [cancelId, setCancelId] = useState('');
  const [modifyId, setModifyId] = useState('');
  const [newPrice, setNewPrice] = useState('');
  const [newQuantity, setNewQuantity] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [lastOrder, setLastOrder] = useState(null);
  const [wsMessage, setWsMessage] = useState('');

  const fetchOrderCount = async () => {
    try {
      const response = await fetch('http://localhost:18080/order_count');
      if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
      const data = await response.json();
      setOrderCount(data.order_count);
      setError(null);
    } catch (err) {
      console.error('Error fetching order count:', err);
      setError('Failed to fetch order count');
    }
  };

  useEffect(() => {
    fetchOrderCount();
    const interval = setInterval(fetchOrderCount, 5000);
    return () => clearInterval(interval);
  }, []);

  useEffect(() => {
    // Create a WebSocket connection for real-time updates.
    const ws = new WebSocket('ws://localhost:18080/ws');
    ws.onopen = () => {
      console.log('WebSocket connected');
    };
    ws.onmessage = (event) => {
      setWsMessage(event.data);
    };
    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };
    ws.onclose = () => {
      console.log('WebSocket closed');
    };
    return () => ws.close();
  }, []);

  const addOrder = async () => {
    setLoading(true);
    setError(null);
    const newOrder = {
      id: Math.floor(Math.random() * 10000),
      price: parseFloat((Math.random() * 100 + 50).toFixed(2)),
      quantity: Math.floor(Math.random() * 100 + 1),
      side: Math.random() < 0.5 ? 'B' : 'S'
    };
    try {
      const response = await fetch('http://localhost:18080/add_order', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(newOrder)
      });
      if (!response.ok) throw new Error(`Failed to add order: ${response.status}`);
      setLastOrder(newOrder);
      await fetchOrderCount();
    } catch (err) {
      console.error('Error adding order:', err);
      setError('Failed to add order');
    } finally {
      setLoading(false);
    }
  };

  const cancelOrder = async () => {
    try {
      const response = await fetch('http://localhost:18080/cancel_order', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id: parseInt(cancelId, 10) })
      });
      const data = await response.json();
      alert(data.message);
      await fetchOrderCount();
    } catch (err) {
      console.error('Error cancelling order:', err);
    }
  };

  const modifyOrder = async () => {
    try {
      const response = await fetch('http://localhost:18080/modify_order', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          id: parseInt(modifyId, 10),
          new_price: parseFloat(newPrice),
          new_quantity: parseInt(newQuantity, 10)
        })
      });
      const data = await response.json();
      alert(data.message);
      await fetchOrderCount();
    } catch (err) {
      console.error('Error modifying order:', err);
    }
  };

  return (
    <div style={{ padding: "2rem", maxWidth: "800px", margin: "0 auto" }}>
      <h1>Trading Simulator Dashboard</h1>
      <p style={{ fontSize: "1.2rem" }}>
        Current Order Count: <strong>{orderCount}</strong>
      </p>
      <p style={{ fontSize: "1rem", color: "#007700" }}>
        WebSocket Update: {wsMessage}
      </p>
      <button onClick={addOrder} disabled={loading} style={{
          padding: '12px 24px',
          fontSize: '16px',
          cursor: loading ? 'not-allowed' : 'pointer',
          backgroundColor: loading ? '#cccccc' : '#4CAF50',
          color: 'white',
          border: 'none',
          borderRadius: '4px',
          marginBottom: '16px',
          transition: 'background-color 0.3s'
        }}>
        {loading ? 'Adding Order...' : 'Add Random Order'}
      </button>
      <hr />
      <div>
        <h3>Cancel Order</h3>
        <input type="number" placeholder="Order ID" value={cancelId} onChange={e => setCancelId(e.target.value)} />
        <button onClick={cancelOrder}>Cancel Order</button>
      </div>
      <div>
        <h3>Modify Order</h3>
        <input type="number" placeholder="Order ID" value={modifyId} onChange={e => setModifyId(e.target.value)} />
        <input type="number" placeholder="New Price" value={newPrice} onChange={e => setNewPrice(e.target.value)} />
        <input type="number" placeholder="New Quantity" value={newQuantity} onChange={e => setNewQuantity(e.target.value)} />
        <button onClick={modifyOrder}>Modify Order</button>
      </div>
      {error && (
        <p style={{
          color: 'red',
          padding: '10px',
          backgroundColor: '#ffebee',
          borderRadius: '4px',
          marginTop: '10px'
        }}>
          {error}
        </p>
      )}
      {lastOrder && (
        <div style={{ marginTop: '20px' }}>
          <h3>Last Added Order Details:</h3>
          <pre style={{
            backgroundColor: '#f5f5f5',
            padding: '15px',
            borderRadius: '4px',
            overflow: 'auto'
          }}>
            {JSON.stringify(lastOrder, null, 2)}
          </pre>
        </div>
      )}
    </div>
  );
}

export default App;

