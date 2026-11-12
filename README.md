# ♟️ Chess Master

> *"Challenge the Future, One Move at a Time."*

**An AI-powered robotic chessboard bridging the gap between physical reality and digital play.**

🌐 **Live site:** [chessmaster26.lol](https://chessmaster26.lol) · 📄 **Docs:** [English](docs/DocsENG.pdf) · [Italiano](docs/DocsITA.pdf)

---

## Overview

Chess Master started from a simple wish: to feel again what it's like to physically touch and move real pieces, something digital chess can't give you.

You play on a real board. Sensors under the squares detect your move, a server checks it and picks a reply (from the Stockfish engine or from a human opponent online), and a robotic arm with an electromagnet moves the opponent's piece for you. A 20×4 LCD and a joystick run **ChessOS**, the board's on-device menu system, and the web app lets anyone join a game from a browser.

## Features

- **Physical play against the AI**: play Stockfish on a real board, with 15 difficulty levels. Lower levels sometimes play a random legal move, so beginners have a real chance.
- **Online play**: create a game and get a 6-character code (e.g. `A7K3B9`). A second player joins from the browser at `chessmaster26.lol/<CODE>`.
- **Local mode**: a browser-only mode at `/LOCAL`.
- **Live move analysis**: every move gets a label (Brilliant, Best Move, Great, Excellent, Good, Inaccuracy, Mistake, Miss, Blunder) based on how much win chance it gains or loses.
- **Opening recognition**: known positions are named using `openings.json`, built from the ECO opening database.
- **Game replays**: finished games are saved and can be replayed by code for 7 days.
- **Robotic arm**: a wooden four-bar linkage driven by servomotors, plus a vertical axis that lowers an electromagnet on a motorized nylon thread to pick up pieces.
- **ChessOS**: menus, game modes, settings and live feedback on the board's LCD, controlled with an analog joystick.
- **Wi-Fi and Bluetooth LE**: the board connects to the server over WebSocket, or pairs over BLE as "Chess Master".
- **Installable web app**: includes a web manifest and service worker.

## How it works

```
 ┌──────────────┐  Serial  ┌──────────────────┐  WebSocket / BLE  ┌──────────────────┐
 │ Arduino Mega │ ◄──────► │ Arduino (Wi-Fi + │ ◄───────────────► │  Bun server      │
 │ sensors, LCD,│          │ BLE bridge)      │                   │  index.js :1707  │
 │ joystick,    │          └──────────────────┘                   │  chess.js rules  │
 │ servos, arm  │                                                 │  Stockfish API   │
 └──────────────┘                                                 └────────┬─────────┘
                                                                           │ WebSocket
                                                                  ┌────────▼─────────┐
                                                                  │ Browser players  │
                                                                  │ game.html etc.   │
                                                                  └──────────────────┘
```

1. The **Mega** scans the 8×8 square matrix. When a piece is lifted from one square and placed on another, it sends `move e2e4` to the Wi-Fi board.
2. The **Wi-Fi board** forwards the move to the server over WebSocket (or over BLE when paired).
3. The **server** checks the move with `chess.js`, sends back the analysis label, and replies with the AI's move or the online opponent's move.
4. The Mega receives the reply and the **arm** carries out the move on the board.

## Repository structure

```
chessmaster/
├── arduino/
│   ├── mega/            # Arduino Mega firmware: board sensing, ChessOS, servos
│   │   ├── mega.ino     # Main loop: scans the 8×8 matrix, detects moves
│   │   ├── menu.h       # ChessOS menus on the 20×4 LCD and joystick
│   │   ├── pins.h       # Pin assignments
│   │   ├── servo.h      # Arm and servomotor control
│   │   └── utils.h      # Helpers
│   └── wifi/            # Wi-Fi/BLE bridge firmware
│       ├── wifi.ino     # WebSocket client, BLE service, Wi-Fi setup
│       └── led.h        # LED animations
├── docs/
│   ├── DocsENG.pdf      # Project documentation (English)
│   └── DocsITA.pdf      # Project documentation (Italian)
├── public/              # Static assets: piece images, styles, font,
│                        # photos, demo video, manifest, service worker
├── index.html           # Landing page
├── game.html            # Online game and replay page
├── local.html           # Local mode
├── about.html           # Project presentation
├── credits.html         # Team and credits
├── index.js             # Express + WebSocket server (game logic, AI, replays)
├── openings.json        # Opening names by position
├── package.json
└── bun.lock
```

## Setup (server and web app)

### Requirements

- [Bun](https://bun.sh)
- Internet access (the AI uses the public [stockfish.online](https://stockfish.online) API)

### Install and run

```bash
git clone https://github.com/raffreddat0/chessmaster.git
cd chessmaster
bun install
```

Create a `.env` file in the project root with the shared secret the physical board uses to log in:

```env
auth=your-secret-token
```

Start the server:

```bash
bun start
```

The server listens on **http://localhost:1707**.

### Routes

| Route | Description |
| --- | --- |
| `/` | Landing page |
| `/ONLINE` | Start a new online game |
| `/<CODE>` | Join or replay a game by its code |
| `/LOCAL` | Local mode |
| `/about` · `/credits` | Project info |
| `/docs` · `/docs?lang=it` | Documentation PDF (English / Italian) |
| `/api/games/:code` | Check whether a game exists |
| `/api/replays` · `/api/replays/:code` | List saved games / get one game's moves |

Finished games are stored in `games.json`, which is created automatically and cleared of games older than 7 days.

## Hardware notes

The board runs on **two microcontrollers** connected by serial at 9600 baud.

### Arduino Mega (`arduino/mega`)

- **Board sensing**: an 8×8 matrix with columns as outputs and rows as `INPUT_PULLUP` inputs, scanned every 500 ms. A move is detected when a square goes from occupied to empty and then another square goes from empty to occupied. It is sent as `move <from><to>` on `Serial1`.
- **Arm**: 4 servomotors drive the four-bar linkage and the vertical axis. An electromagnet picks up the pieces, so each piece needs a magnetic base.
- **UI**: 20×4 LCD and analog joystick for ChessOS.
- Pin assignments are in `pins.h`.

### Wi-Fi/BLE bridge (`arduino/wifi`)

- Written for an Arduino with built-in Wi-Fi, BLE and RTC (it uses `WiFi`, `ArduinoBLE`, `RTC.h` and `WiFiSSLClient`, e.g. the **Arduino UNO R4 WiFi**).
- Talks to the Mega over `SoftwareSerial` on pins **2 (RX) and 3 (TX)**.
- Required libraries: **ArduinoBLE** and **WebSockets** (`WebSocketsClient`).
- Create `arduino/wifi/config.h` with the same secret as the server's `.env`:

  ```cpp
  #define AUTH "your-secret-token"
  ```

- The server IP is saved in EEPROM. Set it with the `ip <address>` serial command. The firmware can also look up `ws.chessmaster<YY>.lol` based on the current year.

**Serial commands** (from the Mega or the USB serial monitor):

| Command | Effect |
| --- | --- |
| `wifi` | Scan and list nearby networks |
| `wifi <ssid>:<password>` | Connect to Wi-Fi and open the WebSocket |
| `ip` / `ip <address>` | Show / set the server IP |
| `ble` | Turn BLE advertising on or off |
| `start` · `exit` | Start / end a game |
| `level <1–15>` | Set the AI difficulty |
| `move <from><to>` | Send a move (e.g. `move e2e4`) |

For wiring diagrams and full build details, see the [documentation](docs/DocsENG.pdf).

## Credits

**Chess Master** was shown at:

- Campionati di Robotica 2026
- HoPE 2024/2025
- Maker Faire 2025 (Rome)
- Notte della Ricerca 2025/2026 (Lecce)

**Created by**
Giovanni Paolo Montagna · Matteo Geusa · Samuele Putignani · Lorenzo Afrune · Camilla Torresin · Paola Candido · Alessandro Meraglia · Giulia Levanto

**External help**
Niccolò Amato · Andrea Mangia

**Special thanks to** Frankie & Franco

### Built with

[chess.js](https://github.com/jhlywa/chess.js) · [Stockfish](https://stockfishchess.org) via [stockfish.online](https://stockfish.online) · [Express](https://expressjs.com) · [ws](https://github.com/websockets/ws) · [Bun](https://bun.sh)

## License

ISC (as declared in `package.json`).