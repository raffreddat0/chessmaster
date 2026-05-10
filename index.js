const express = require("express");
const path = require("path");
const http = require("http");
const { Server } = require("ws");
const Chess = require("chess.js").Chess;
require("dotenv").config();

const app = express();
const port = 1707;

app.use(express.static(path.join(__dirname, "public")));

app.get("/api/games/:code", (req, res) => {
  const code = (req.params.code || "").toString().trim().toUpperCase();
  if (!code) return res.json({ exists: false });
  if (code === "ONLINE") return res.json({ exists: true });

  const exists = games.includes(code) || (sessions[code] && sessions[code].length === 2);
  return res.json({ exists });
});

app.get("/credits", (req, res) => {
  res.sendFile(path.join(__dirname, "./credits.html"));
});

app.get("*", (req, res) => {
  res.sendFile(path.join(__dirname, "./index.html"));
});

const server = http.createServer(app);
const wss = new Server({ server });

function generateCode() {
  const letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const numbers = "0123456789";

  let codeArray = [];
  for (let i = 0; i < 3; i++) {
    const randomIndex = Math.floor(Math.random() * letters.length);
    codeArray.push(letters[randomIndex]);
  }

  for (let i = 0; i < 3; i++) {
    const randomIndex = Math.floor(Math.random() * numbers.length);
    codeArray.push(numbers[randomIndex]);
  }

  for (let i = codeArray.length - 1; i > 0; i--) {
    const j = Math.floor(Math.random() * (i + 1));
    [codeArray[i], codeArray[j]] = [codeArray[j], codeArray[i]];
  }

  return codeArray.join("");
}

const getStockfishMove = async (fen, level) => {
  const url = "https://stockfish.online/api/s/v2.php";
  const params = new URLSearchParams({
    fen: fen,
    depth: Math.max(Math.min(level + 3, 15), 5)
  });

  try {
    const response = await fetch(`${url}?${params}`);
    const data = await response.json();

    if (data.success) {
      const continuation = data.continuation.split(" ");
      return continuation[0];
    } else {
      throw new Error("Errore nell\"analisi della posizione");
    }
  } catch (error) {
    console.error("Errore:", error);
    await new Promise(r => setTimeout(r, 2000));
    return null;
  }
};

let games = [];
let sessions = {};
wss.on("connection", (ws, req) => {
  const urlParams = new URLSearchParams(req.url.split("?")[1]);
  let code = urlParams.get("code");
  let auth = urlParams.get("auth");

  if (!code && !auth) {
    ws.close();
    return;
  }

  if (code && !games.includes(code) && code !== "ONLINE" && (!sessions[code] || sessions[code]?.length === 1)) {
    ws.send("error");
    ws.close();
    return;
  }

  if (auth && process.env.auth !== auth) {
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
  } else
    ws.send("time " + Date.now());

  console.log("Client connesso");

  let pongTimeout;
  const heartbeat = () => {
    if (ws.readyState === ws.OPEN) {
      ws.ping();

      pongTimeout = setTimeout(() => {
        try {
          ws.terminate();
        } catch { }
      }, 5000);
    }
  };

  const heartbeatInterval = setInterval(heartbeat, 30000);

  ws.on("pong", (data) => {
    clearTimeout(pongTimeout);
  });

  let level = 1;
  let chess = new Chess();
  let timer = null;
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
    ws.on("message", async (data) => {
      const message = data.toString();
      if (message.startsWith("level")) {
        level = parseInt(message.split(" ")[1]) || 1;
        level = Math.max(level, 1);
        level = Math.min(level, 15);
        console.log(`Livello impostato a ${level}`);
        return;
      }

      if (sessions[game]?.length === 2 && message === "history") {
        const moves = chess.history({ verbose: true }).map((m) => {
          const promo = m.promotion ? String(m.promotion) : "";
          return `${m.from}${m.to}${promo}`;
        });
        sessions[game][1].emit("message", "history " + JSON.stringify(moves));
        return;
      }

      if (message.startsWith("timer")) {
        const timestamp = message.replace("timer", "").trim();
        if (timer && !timestamp)
          sessions[game][1].emit("message", "timer " + timer);
        else if (timestamp)
          timer = Number(timestamp);
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
        let piece;
        try {
          piece = chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: "q" }).piece;
        } catch (error) {
          sessions[game][1].emit("message", "invalid");
          return;
        }

        wait = false;
        ws.send(auth === "online" ? message : (piece + message));
        if (chess.isGameOver())
          ws.send(chess.isDraw() ? "draw" : "lose");
        return;
      }

      console.log(`Mossa dell"utente: ${message}`);
      try {
        chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: "q" });
      } catch (error) {
        ws.send("invalid");
        return;
      }

      ws.send("valid");
      if (chess.isGameOver()) {
        ws.send(chess.isDraw() ? "draw" : "win");
      }

      if (sessions[game].length === 2) {
        sessions[game][1].emit("message", message);
        wait = true;
        return;
      }

      if ((chess.history().length > 1 && games.includes(game)) || chess.isGameOver())
        return;

      if (games.includes(game)) {
        ws.send("stockfish");
        timer = Date.now();
        ws.send("timer " + timer);
        games = games.filter(code => code !== game);
      }

      async function stockfish() {
        try {
          const legalMoves = chess.moves({ verbose: true });
          const mistakeChance = Math.max(0.6 - level * 0.08, 0.05);

          let chosen;
          if (Math.random() < mistakeChance) {
            chosen = legalMoves[Math.floor(Math.random() * legalMoves.length)];
            await new Promise(r => setTimeout(r, 500));
          } else {
            const bestMove = await getStockfishMove(chess.fen(), level);
            const from = bestMove.substring(0, 2);
            const to = bestMove.substring(2, 4);

            chosen = { from, to, promotion: "q" };
          }

          const result = chess.move(chosen);
          if (!result) throw new Error("invalid");

          const uci = `${chosen.from}${chosen.to}${chosen.promotion && chosen.promotion !== "q" ? chosen.promotion : ""}`;
          const piece = result.piece;

          console.log(`Mossa di stockfish: ${uci}`);
          ws.send(auth === "online" ? uci : (piece + uci));
        } catch {
          return await stockfish();
        }
      }

      await stockfish();
      if (chess.isGameOver()) {
        ws.send(chess.isDraw() ? "draw" : "lose");
        return;
      }
    });
  else {
    ws.on("message", async (data) => {
      const message = data.toString();

      if (message.startsWith("history")) {
        try {
          const payload = JSON.parse(message.replace("history ", ""));
          if (Array.isArray(payload)) {
            chess = new Chess();
            for (const m of payload) {
              if (typeof m !== "string") continue;
              const from = m.slice(0, 2);
              const to = m.slice(2, 4);
              const promotion = m.length > 4 ? m.slice(4, 5) : "q";
              chess.move({ from, to, promotion });
            }
          } else if (typeof payload === "string") {
            chess.load(payload);
          }
        } catch { }
        wait = chess.turn() === "w";
        ws.send(message);
        return;
      }

      if (message.startsWith("timer ")) {
        if (timer) return;
        timer = Number(message.replace("timer ", ""));
        ws.send(message);
        return;
      }

      if (wait) {
        try {
          chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: "q" });
        } catch (error) {
          console.log(error.description);
          sessions[game][0].emit("message", "invalid");
          return;
        }

        wait = false;
        ws.send(message);

        if (chess.isGameOver())
          ws.send(chess.isDraw() ? "draw" : "lose");
        return;
      }

      console.log(`Mossa dell"avversario: ${message}`);
      try {
        chess.move({ from: message.substring(0, 2), to: message.substring(2, 4), promotion: "q" });
      } catch (error) {
        console.log(error.description);
        ws.send("invalid");
        return;
      }

      ws.send("valid");
      if (chess.isGameOver()) {
        ws.send(chess.isDraw() ? "draw" : "win");
      }

      if (sessions[game].length === 2) {
        sessions[game][0].emit("message", message);
        wait = true;
        return;
      }

    });
    sessions[game][0].emit("message", "history");
    sessions[game][0].emit("message", "timer");
  }

  ws.on("close", () => {
    console.log("Client disconnesso");
    if (!code) {
      if (sessions[game]?.length === 2)
        sessions[game][1].close();
      sessions[game] = [];
      games = games.filter(code => code !== game);
      game = null;
    } else
      sessions[game] = [sessions[game][0]];
    clearInterval(heartbeatInterval);
    clearTimeout(pongTimeout);
  });
});

server.listen(port, "0.0.0.0", () => {
  console.log(`Server in ascolto sulla porta http://localhost:${port}`);
});

process.on("uncaughtException", (error) => {
  console.error(error);
});

process.on("unhandledRejection", (reason) => {
  console.error(reason);
});
