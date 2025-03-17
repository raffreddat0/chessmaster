const express = require('express');
const path = require('path');
const https = require('https');
const http = require('http');
const { Server } = require('ws');
const { exec } = require('child_process');
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
  const characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  let code = '';
  for (let i = 0; i < 6; i++) {
    const randomIndex = Math.floor(Math.random() * characters.length);
    code += characters[randomIndex];
  }
  return code;
}

const getStockfishMove = (fen, level) => {
  return new Promise((resolve, reject) => {
    const stockfishProcess = exec('stockfish');
    const depth = level > 5 ? level : 5;

    stockfishProcess.stdin.write('uci\n');
    stockfishProcess.stdin.write('setoption name Threads value 4\n');
    stockfishProcess.stdin.write('setoption name Hash value 1024\n');
    stockfishProcess.stdin.write(`setoption name Skill Level value ${level}\n`);
    stockfishProcess.stdin.write(`position fen ${fen}\n`);
    stockfishProcess.stdin.write(`go depth ${depth}\n`);

    stockfishProcess.stdout.on('data', (data) => {
      const output = data.toString();
      console.log(output);
      if (output.includes('bestmove')) {
        const args = output.split(' ');
        const index = args.indexOf(args.find(arg => arg.includes('bestmove'))) + 1;
        resolve(args[index]);
      }
    });

    stockfishProcess.on('error', (error) => {
      reject(error);
    });

    stockfishProcess.on('close', (code) => {
      if (code !== 0) {
        reject(new Error(`Stockfish process ended with code ${code}`));
      }
    });
  });
};

function withTimeout(fun, timeout) {
  const timeoutPromise = new Promise((_, reject) => 
    setTimeout(() => reject(new Error("Timeout!")), timeout)
  );

  return Promise.race([fun(), timeoutPromise])
    .catch((err) => {
      console.error(err.message);
      return withTimeout(fun, timeout);
      });
}

let game = "";
let sessions = [];
let mode = 0;
wss.on('connection', (ws, req) => {
  const urlParams = new URLSearchParams(req.url.split('?')[1]);
  let code = urlParams.get('code');
  let auth = urlParams.get('auth');

  if ((!code && !auth) || mode === 1 || sessions.length === 2) {
    ws.close();
    return;
  }
  
  if (code && game !== code && code !== "chess") {
    console.log("codice non valido");
    ws.close();
    return;
  }

  if (auth && process.env.auth !== auth) {
    console.log("auth non valido");
    ws.close();
    return;
  }

  if (code === "chess") {
    code = null;
    auth = "chess";
  }

  if (code) {
    sessions[1] = ws;
    mode = 2;
    ws.send("start");
  } else
    sessions[0] = ws;

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

  let level = 5;
  let chess = new Chess();
  let wait = code ? true : false;

  if (auth === "chess") {
    chess = new Chess();
    wait = false;
    mode = 0;
    if (!code)
      game = generateCode();

    console.log("Partita create con codice", game);
    ws.send("code " + game);
  }

  if (!code)
  ws.on('message', async (data) => {
    const message = data.toString();
    if (message.startsWith("level")) {
      level = parseInt(message.split(" ")[1]);
      console.log(`Livello impostato a ${level}`);
      return;
    }

    if (sessions.length === 2 && message === "position"){
      sessions[1].emit("message", "position " + chess.fen());
      return;
    }

    if (message === "start") {
      chess = new Chess();
      wait = false;
      mode = 0;
      if (!code)
        game = generateCode();

      if (sessions.length === 2) {
        sessions[1].close();
        sessions = [sessions[0]];
      }

      console.log("Partita create con codice", game);
      ws.send("code " + game);
      return;
    }

    if (message === "end") {
      console.log("Partita terminata");
      game = "";
      mode = 0;
      wait = false;
      sessions[1].close();
      sessions = [sessions[0]];
      return;
    }

    if (wait) {
      try {
        chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: 'q' });
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
      chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: 'q' });
    } catch(error) {
      console.log(error.description);
      ws.send('error');
      return;
    }

    if (chess.isGameOver())
      ws.send(chess.isDraw() ? "draw" : "win");

    if (sessions.length === 2) {
      sessions[1].emit('message', message);
      wait = true;
      return;
    }

    if (mode === 0) {
      ws.send("stockfish");
      mode = 1;
    }

    async function stockfish() {
      try {
        //const stockfishMove = await withTimeout(getStockfishMove.bind(null, moves, level), 2000);
        const stockfishMove = await getStockfishMove(chess.fen(), level);
        console.log(`Mossa di Stockfish: ${stockfishMove}`);
        chess.move({ from: stockfishMove.substring(0, 2), to: stockfishMove.substring(2, 4), promotion: 'q' });
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
        chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: 'q' });
      } catch(error) {
        console.log(error.description);
        sessions[0].emit('message', 'error');
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
      chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: 'q' });
    } catch(error) {
      console.log(error.description);
      ws.send('error');
      return;
    }

    if (chess.isGameOver()) {
      ws.send(chess.isDraw() ? "draw" : "win");
    }

    if (sessions.length === 2) {
      sessions[0].emit('message', message);
      wait = true;
      return;
    }

  });
  sessions[0].emit('message', 'position');
  }

  ws.on('close', () => {
    console.log('Client disconnesso');
    if (!code) {
      game = "";
      mode = 0;
      if (sessions[1])
        sessions[1].close();
      sessions = [];
    } else
      sessions = [sessions[0]];
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