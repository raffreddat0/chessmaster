const express = require('express');
const path = require('path');
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
  
  if (code && !games.includes(code) && code !== "online" && sessions[code].length === 1) {
    console.log("codice non valido");
    ws.close();
    return;
  }

  if (auth && process.env.auth !== auth) {
    console.log("auth non valido");
    ws.close();
    return;
  }

  if (code === "online") {
    code = null;
    auth = "online";
  }

  if (code) {
    sessions[code].push(ws);
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

  let level = 5;
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
      level = parseInt(message.split(" ")[1]);
      console.log(`Livello impostato a ${level}`);
      return;
    }

    if (sessions[game].length === 2 && message === "position"){
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

    if (message === "end") {
      console.log("Partita terminata");
      wait = false;
      sessions[game][1].close();
      sessions[game] = [ws];
      games = games.filter(code => code !== game);
      game = "";
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
      if (sessions[game].length === 2)
        sessions[game][1].close();
      sessions[game] = [];
      games = games.filter(code => code!== game);
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