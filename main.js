const express = require('express');
const http = require('http');
const { Server } = require('ws');

const app = express();
const port = 1707;

app.use(express.json());

app.get('/', (req, res) => {
  res.json({ "txt": "ciao" });
});

const server = http.createServer(app);
const wss = new Server({ server });

wss.on('connection', (ws) => {
  console.log('Client connesso');

  let pongTimeout;
  const heartbeat = () => {
    if (ws.readyState === ws.OPEN) {
      ws.ping();

      pongTimeout = setTimeout(() => {
        try {
          ws.terminate();
        } catch {}
      }, 5000);
    }
  };

  const heartbeatInterval = setInterval(heartbeat, 30000);

  ws.on('pong', (data) => {
    clearTimeout(pongTimeout); 
  });

  ws.on('message', (message) => {
    console.log(`${message}`);
    ws.send(`Echo: ${message}`);
  });

  ws.on('close', () => {
    console.log('Client disconnesso');
    clearInterval(heartbeatInterval);
    clearTimeout(pongTimeout);
  });
});

server.listen(port, '0.0.0.0', () => {
  console.log(`Server in ascolto sulla porta ${port}`);
});

process.on('uncaughtException', (error) => {
  console.error(error);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error(reason);
});