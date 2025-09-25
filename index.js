const express = require('express');
const path = require('path');
const http = require('http');
const { Server } = require('ws');
const Chess = require('chess.js').Chess;
require('dotenv').config();

const app = express();
const port = 1707;

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, './index.html'));
});

app.use((req, res) => {
  res.redirect('/');
});

const server = http.createServer(app);
const wss = new Server({ server });

function generateCode() {
  const characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
  let code = '';
  for (let i = 0; i < 6; i++) {
    const randomIndex = Math.floor(Math.random() * characters.length);
    code += characters[randomIndex];
  }
  return code;
}

const getStockfishMove = async (fen, level) => {
  const url = 'https://stockfish.online/api/s/v2.php';
  const params = new URLSearchParams({
    fen: fen,
    depth: level
  });

  try {
    const response = await fetch(`${url}?${params}`);
    const data = await response.json();

    if (data.success) {
      const continuation = data.continuation.split(" ");
      return continuation[0];
    } else {
      throw new Error('Errore nell\'analisi della posizione');
    }
  } catch (error) {
    console.error('Errore:', error);
    return null;
  }
};

let games = [];
let sessions = {};
wss.on('connection', (ws, req) => {
  const urlParams = new URLSearchParams(req.url.split('?')[1]);
  let code = urlParams.get('code');
  let auth = urlParams.get('auth');

  if (!code && !auth) {
    ws.close();
    return;
  }

  if (code && !games.includes(code) && code !== "ONLINE" && sessions[code].length === 1) {
    console.log("codice non valido");
    ws.close();
    return;
  }

  if (auth && process.env.auth !== auth) {
    console.log("auth non valido");
    ws.close();
    return;
  }

  if (code === "ONLINE") {
    code = null;
    auth = "online";
  }

  if (code) {
    sessions[code].push(ws);
    sessions[code][0].send("joined");
    ws.send("start");
  }

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

  let level = 1;
  let chess = new Chess();
  let wait = code ? true : false;
  let game = code;

  if (auth === "online") {
    chess = new Chess();
    wait = false;
    game = generateCode();
    games.push(game);
    sessions[game] = [ws];

    console.log("Partita create con codice", game);
    ws.send("code " + game);
  }

  if (!code)
  ws.on('message', async (data) => {
    const message = data.toString();
    if (message.startsWith("level")) {
      level = parseInt(message.split(" ")[1]) || 3;
      if (level > 15)
        level = 15;
      console.log(`Livello impostato a ${level}`);
      return;
    }

    if (sessions[game]?.length === 2 && message === "position"){
      sessions[game][1].emit("message", "position " + chess.fen());
      return;
    }

    if (message === "start") {
      chess = new Chess();
      wait = false;
      if (!code) {
        game = generateCode();
        games.push(game);
        sessions[game] = [ws];
      }

      console.log("Partita create con codice", game);
      ws.send("code " + game);
      return;
    }

    if (message === "exit") {
      console.log("Partita terminata");
      wait = false;
      if (sessions[game]?.length === 2)
        sessions[game][1].close();
      sessions[game] = [];
      games = games.filter(code => code !== game);
      game = null;
      return;
    }

    if (wait) {
      try {
        chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: message.substring(4, 5) || 'q' });
      } catch(error) {
        console.log(error.description);
        sessions[1].emit('message', 'error');
        return;
      }

      wait = false;
      ws.send(message);
      if (chess.isGameOver())
        ws.send(chess.isDraw() ? "draw" : "lose");
      return;
    }

    console.log(`Mossa dell'utente: ${message}`);
    try {
      chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: message.substring(4, 5) || 'q' });
    } catch(error) {
      console.log(error.description);
      ws.send('error');
      return;
    }

    if (chess.isGameOver())
      ws.send(chess.isDraw() ? "draw" : "win");

    if (sessions[game].length === 2) {
      sessions[game][1].emit('message', message);
      wait = true;
      return;
    }

    if (chess.history().length > 1 && games.includes(game))
      return;

    if (games.includes(game)) {
      ws.send("stockfish");
      games = games.filter(code => code !== game);
    }

    async function stockfish() {
      try {
        const stockfishMove = await getStockfishMove(chess.fen(), level);
        console.log(`Mossa di Stockfish: ${stockfishMove}`);
        chess.move({ from: stockfishMove.substring(0, 2), to: stockfishMove.substring(2, 4), promotion: message.substring(4, 5) || 'q' });
        ws.send(stockfishMove);
      } catch {
        return await stockfish();
      }
    }

    await stockfish();
    if (chess.isGameOver())
      ws.send(chess.isDraw() ? "draw" : "lose");
  });
  else {
  ws.on('message', async (data) => {
    const message = data.toString();

    if (message.startsWith("position")) {
      chess.load(message.replace("position ", ""));
      wait = chess.turn() === "w";
      ws.send(message);
      return;
    }

    if (wait) {
      try {
        chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: message.substring(4, 5) || 'q' });
      } catch(error) {
        console.log(error.description);
        sessions[game][0].emit('message', 'error');
        return;
      }

      wait = false;
      ws.send(message);

      if (chess.isGameOver())
        ws.send(chess.isDraw() ? "draw" : "lose");
      return;
    }

    console.log(`Mossa dell'avversario: ${message}`);
    try {
      chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: message.substring(4, 5) || 'q' });
    } catch(error) {
      console.log(error.description);
      ws.send('error');
      return;
    }

    if (chess.isGameOver()) {
      ws.send(chess.isDraw() ? "draw" : "win");
    }

    if (sessions[game].length === 2) {
      sessions[game][0].emit('message', message);
      wait = true;
      return;
    }

  });
  sessions[game][0].emit('message', 'position');
  }

  ws.on('close', () => {
    console.log('Client disconnesso');
    if (!code) {
      if (sessions[game]?.length === 2)
        sessions[game][1].close();
      sessions[game] = [];
      games = games.filter(code => code!== game);
      game = null;
    } else
      sessions[game] = [sessions[game][0]];
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

process.on('unhandledRejection', (reason) => {
  console.error(reason);
});
