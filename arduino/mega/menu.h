#include <EEPROM.h>
#include <LiquidCrystal.h>

#include "servo.h"
#include "utils.h"

void animation(int type);
void lcdbegin();

struct Config {
  int level;
  int lang;
  char ssid[32];
  int streak;
  int wins;
  int losses;
  int draws;
  int games;
};

Config config;
Square T[8][8] = {};
Square T0 = {};
int addr = sizeof(config);
int addr0 = addr + sizeof(S);
String ip;
String ip0;

int value1 = 0;
int value2 = 0;
int click = 0;
int prevent = 0;

int x0 = 0;
int y0 = 0;
int z0 = 0;
int x = 0;
int y = 0;
int z = 0;

int r = 0;
int e = 0;
int f = 0;
int s = 0;

String input = "";
int page = -2;
int status = 0;
int confirm = 0;
int yes = 0;
int size = -1;
int editing = 0;

char wifis[10][20];
char tssid[20] = "";
int redirect = 0;
int scanning = 0;
bool connected = false;
bool stockfish = false;

unsigned long gap = 0;
unsigned long timer = 0;
unsigned long time = 0;
char code[7] = "";
int playing = -1;

void ditch() {
  const char *settings[] = {
      "Turn: ", "Move: ", "Down: ", "Try", "Save", "Exit"};

  const int numItems = sizeof(settings) / sizeof(settings[0]);

  if (!editing) {
    T0 = S0;
    editing = 1;
  }

  if (y != y0 || x != x0) {
    lcd.clear();
    delay(100);
    y = constrain(y, 0, numItems - 1);
  }

  char square[3] = "";
  int range[4] = {1, 5, 10, 20};

  lcd.setCursor(7, 0);
  lcd.print("Ditch");
  lcd.setCursor(17, 0);
  if (range[r] < 10) {
    lcd.print(" ");
    lcd.print(range[r]);
  } else
    lcd.print(range[r]);

  int start = numItems - 1 == y ? max(0, y - 2) : max(0, y - 1);
  int end = min(start + 3, numItems);

  if (T0.turn < 0)
    T0.turn = 90;
  if (T0.move < 0)
    T0.move = 0;
  if (T0.down < 0)
    T0.down = 0;

  for (int i = start; i < end; i++) {
    int row = i - start + 1;
    lcd.setCursor(0, row);

    if (i == y)
      lcd.print("> ");
    else
      lcd.print("  ");

    lcd.print(settings[i]);
    if (strcmp(settings[i], "Turn: ") == 0) {
      if (i == y && x != x0) {
        T0.turn += range[r] * (x - x0);
        if (T0.turn < 0)
          T0.turn = 180;
        if (T0.turn > 180)
          T0.turn = 0;
        x = x0 = 0;
      }

      lcd.print("<");
      lcd.print(T0.turn);
      lcd.print(">");
    }

    if (strcmp(settings[i], "Move: ") == 0) {
      if (i == y && x != x0) {
        T0.move += range[r] * (x - x0);
        if (T0.move < 0)
          T0.move = 180;
        if (T0.move > 180)
          T0.move = 0;
        x = x0 = 0;
      }

      lcd.print("<");
      lcd.print(T0.move);
      lcd.print(">");
    }

    if (strcmp(settings[i], "Down: ") == 0) {
      if (i == y && x != x0) {
        T0.down += range[r] * (x - x0);
        T0.down = constrain(T0.down, 0, 150);
        x = x0 = 0;
      }

      lcd.print("<");
      lcd.print(T0.down * 100);
      lcd.print(">");
    }

    if (strcmp(settings[i], "Save") == 0) {
      if (S0.turn == T0.turn && S0.move == T0.move && S0.down == T0.down)
        lcd.print("d");
      else
        lcd.print(" ");
    }
  }

  if (click) {
    click = 0;
    prevent = 1;
    if (y == numItems - 3) {
      reset();
      turn(T0.turn);
      move(T0.move);
      down(T0.down * 100);
    } else if (y == numItems - 2) {
      S0 = T0;
      EEPROM.put(addr0, S0);
    } else if (y == numItems - 1) {
      page = redirect;
      x = x0 = 0;
      y = y0 = 2;
      reset();
      editing = 0;
      lcd.clear();
      delay(100);
    } else {
      r++;
      if (r > 3)
        r = 0;
    }
  }
}

void calibration() {
  lcd.setCursor(4, 0);
  lcd.print("Calibration");

  int ms[4] = {100, 200, 500, 1000};
  if (x != x0 && e) {
    if (x > 4)
      x = 0;
    if (x < 0)
      x = 4;

    f = x;
  }

  if (f < 4) {
    lcd.setCursor(6, 2);
    lcd.print(e ? "<" : " ");
    lcd.print(ms[f]);
    if (e)
      lcd.print(">");
    lcd.print(" ms   ");
  } else {
    lcd.setCursor(6, 2);
    lcd.print(" <Exit>  ");
  }

  if (y != y0 && f < 4) {
    y = constrain(y, -1, 1);

    if (y == 1) {
      down(ms[f]);
    }
    if (y == -1) {
      up(ms[f]);
    }

    y = y0 = 0;
  }

  if (click) {
    click = 0;
    prevent = 1;
    if (x < 4) {
      if (e)
        e = 0;
      else
        e = 1;
    } else {
      page = redirect;
      e = f = 0;
      x = x0 = 0;
      y = y0 = 1;
      lcd.clear();
      delay(100);
    }
  }
}

void squares() {
  const char *settings[] = {
      "Square: ", "Turn: ", "Move: ", "Down: ", "Try", "Save", "Exit"};

  const int numItems = sizeof(settings) / sizeof(settings[0]);

  if (!editing) {
    memcpy(T, S, sizeof(S));
    editing = 1;
  }

  if (y != y0 || x != x0) {
    lcd.clear();
    delay(100);
    y = constrain(y, 0, numItems - 1);
  }

  char square[3] = "";
  int range[4] = {1, 5, 10, 20};

  lcd.setCursor(6, 0);
  lcd.print("Squares");
  lcd.setCursor(17, 0);
  if (range[r] < 10) {
    lcd.print(" ");
    lcd.print(range[r]);
  } else
    lcd.print(range[r]);

  int start = numItems - 1 == y ? max(0, y - 2) : max(0, y - 1);
  int end = min(start + 3, numItems);

  if (T[7 - z][z0].turn < 0)
    T[7 - z][z0].turn = 90;
  if (T[7 - z][z0].move < 0)
    T[7 - z][z0].move = 0;
  if (T[7 - z][z0].down < 0)
    T[7 - z][z0].down = 0;

  pinMode(pinMagnet, 1);

  for (int i = start; i < end; i++) {
    int row = i - start + 1;
    lcd.setCursor(0, row);

    if (i == y)
      lcd.print("> ");
    else
      lcd.print("  ");

    lcd.print(settings[i]);
    if (strcmp(settings[i], "Square: ") == 0) {
      if (i == y && x != x0) {
        if (s == 1)
          z0 += (x - x0);
        else
          z += (x - x0);

        if (z < 0) {
          z = 7;
          if (s == 0)
            z0--;
        }
        if (z > 7) {
          z = 0;
          if (s == 0)
            z0++;
        }
        if (z0 < 0)
          z0 = 7;
        if (z0 > 7)
          z0 = 0;

        x = x0 = 0;
      }

      square[0] = char('a' + z0);
      square[1] = char('1' + 7 - (7 - z));
      square[2] = '\0';

      switch (s) {
      case 0:
        lcd.print("<");
        lcd.print(square);
        lcd.print(">");
        break;
      case 1:
        lcd.print("<");
        lcd.print(square[0]);
        lcd.print(">");
        lcd.print(square[1]);
        break;
      case 2:
        lcd.print(square[0]);
        lcd.print("<");
        lcd.print(square[1]);
        lcd.print(">");
        break;
      }
    }

    if (strcmp(settings[i], "Turn: ") == 0) {
      if (i == y && x != x0) {
        T[7 - z][z0].turn += range[r] * (x - x0);
        if (T[7 - z][z0].turn < 0)
          T[7 - z][z0].turn = 180;
        if (T[7 - z][z0].turn > 180)
          T[7 - z][z0].turn = 0;
        x = x0 = 0;
      }

      lcd.print("<");
      lcd.print(T[7 - z][z0].turn);
      lcd.print(">");
    }

    if (strcmp(settings[i], "Move: ") == 0) {
      if (i == y && x != x0) {
        T[7 - z][z0].move += range[r] * (x - x0);
        if (T[7 - z][z0].move < 0)
          T[7 - z][z0].move = 180;
        if (T[7 - z][z0].move > 180)
          T[7 - z][z0].move = 0;
        x = x0 = 0;
      }

      lcd.print("<");
      lcd.print(T[7 - z][z0].move);
      lcd.print(">");
    }

    if (strcmp(settings[i], "Down: ") == 0) {
      if (i == y && x != x0) {
        T[7 - z][z0].down += range[r] * (x - x0);
        T[7 - z][z0].down = constrain(T[7 - z][z0].down, 0, 150);
        x = x0 = 0;
      }

      lcd.print("<");
      lcd.print(T[7 - z][z0].down * 100);
      lcd.print(">");
    }

    if (strcmp(settings[i], "Save") == 0) {
      if (memcmp(S, T, sizeof(S)) == 0)
        lcd.print("d");
      else
        lcd.print(" ");
    }
  }

  if (click) {
    click = 0;
    prevent = 1;
    if (y == numItems - 3) {
      reset();
      turn(T[7 - z][z0].turn);
      move(T[7 - z][z0].move);
      down(T[7 - z][z0].down * 100);
    } else if (y == numItems - 2) {
      memcpy(S, T, sizeof(T));
      EEPROM.put(addr, S);
    } else if (y == numItems - 1) {
      page = redirect;
      y = x = 0;
      y0 = x0 = 0;
      reset();
      pinMode(pinMagnet, 0);
      editing = 0;
      lcd.clear();
      delay(100);
    } else if (y == 0) {
      s++;
      if (s > 2)
        s = 0;
    } else {
      r++;
      if (r > 3)
        r = 0;
    }
  }
}

void ipaddress() {
  if (input == "invalid ip") {
    lcd.noCursor();
    lcd.setCursor(2, 0);
    lcd.print("     Invalid!     ");
    input = "";
    delay(1000);
  }

  if (input == "valid ip") {
    lcd.noCursor();
    lcd.clear();
    delay(100);
    lcd.setCursor(7, 1);
    lcd.print("Saved!");
    delay(1000);
    ip = ip0;
    page = redirect;
    x = x0 = 0;
    y = y0 = 3;
    editing = 0;
    input = "";
    lcd.clear();
  } else {
    if (!editing) {
      ip0 = ip;
      editing = 1;
      y = y0 = 1;
      x = x0 = 0;
    }

    lcd.setCursor(2, 0);
    lcd.print(ip0);
    for (int i = 0; i < 16 - ip0.length(); i++)
      lcd.print(" ");

    lcd.setCursor(5, 2);
    lcd.print("1234567890");
    lcd.setCursor(1, 3);
    lcd.print("CANCEL . DEL ENTER");

    if (y != y0) {
      x = 0;
    }

    y = constrain(y, 0, 1);
    lcd.cursor();
    switch (y) {
      case 0:
      if (x > 9)
        x = 0;
      if (x < 0)
        x = 9;
      lcd.setCursor(5 + x, 2);
      break;
    case 1:
      x = constrain(x, 0, 3);
      switch (x) {
      case 0:
        lcd.setCursor(1, 3);
        break;
      case 1:
        lcd.setCursor(8, 3);
        break;
      case 2:
        lcd.setCursor(10, 3);
        break;
      case 3:
        lcd.setCursor(14, 3);
        break;
      }
      break;
    }
    delay(150);
  }

  if (click) {
    click = 0;
    prevent = 1;
    switch (y) {
    case 0:
      if (ip0.length() < 16) {
        int val = x + 1;
        if (val == 10)
          val = 0;
        ip0 += String(val);
      }
      break;
    case 1:
      switch (x) {
      case 0:
        page = redirect;
        x = x0 = 0;
        y = y0 = 3;
        editing = 0;
        input = "";
        lcd.noCursor();
        lcd.clear();
        delay(100);
        break;
      case 1:
        if (ip0.length() < 16)
          ip0 += '.';
        break;
      case 2:
        if (ip0.length() > 0)
          ip0.remove(ip0.length() - 1);
        break;
      case 3:
        if (ip0.length() < 7)
          input = "invalid ip";
        else
          Serial1.println("ip " + ip0);
        break;
      }
      break;
    }
  }
}

void debug() {
  struct MenuItem {
    const char *label;
    int page;
  };

  const MenuItem settings[] = {
      {"Squares", -5}, {"Calibration", -6}, {"Ditch", -7}, {"IP Address", -8}, {"Exit", 0}};

  const int numItems = sizeof(settings) / sizeof(settings[0]);

  if (y != y0 || x != x0) {
    lcd.clear();
    delay(100);
    y = constrain(y, 0, numItems - 1);
  }

  lcd.setCursor(8, 0);
  lcd.print("Debug");

  int start = numItems - 1 == y ? max(0, y - 2) : max(0, y - 1);
  int end = min(start + 3, numItems);

  for (int i = start; i < end; i++) {
    int row = i - start + 1;
    lcd.setCursor(0, row);

    if (i == y)
      lcd.print("> ");
    else
      lcd.print("  ");

    lcd.print(settings[i].label);
  }

  if (click) {
    click = 0;
    prevent = 1;
    if (settings[y].page) {
      redirect = -4;
      page = settings[y].page;
      x = x0 = 0;
      y = y0 = 0;
      lcd.clear();
      delay(100);
    } else if (y == numItems - 1) {
      page = redirect = 4;
      x = x0 = 0;
      y = y0 = 3;
      lcd.clear();
      delay(100);
    }
  }
}

void credits() {
  const char *credits[] = {
      "Giovanni Montagna", "Matteo Geusa",   "Samuele Putignani",
      "Camilla Torresin",  "Lorenzo Afrune", "Alessandro Meraglia",
      "Paola Candido",     "Niccolo' Amato", "Andrea Mangia", ""};

  const int numItems = sizeof(credits) / sizeof(credits[0]);

  if (y != y0 || x != x0) {
    lcd.clear();
    delay(100);

    y = constrain(y, 0, numItems - 1);
  }

  lcd.setCursor(6, 0);
  lcd.print("Credits");

  int start = numItems - 1 == y ? max(0, y - 2) : max(0, y - 1);
  int end = min(start + 3, numItems);

  for (int i = start; i < end; i++) {
    lcd.setCursor(0, i - start + 1);
    lcd.print(credits[i]);
  }

  if (end == numItems)
    if (ddlay(500)) {
      lcd.setCursor(6, 3);
      if (status % 2 == 0)
        lcd.print("<Exit>");
      else
        lcd.print("           ");
      status = (status + 1) % 2;
    }

  if (click) {
    click = 0;
    prevent = 1;
    page = 4;
    x = x0 = 0;
    y = y0 = 4;
    lcd.clear();
  }
}

void wifi() {
  if (input == "connected") {
    lcd.clear();
    delay(100);
    lcd.setCursor(5, 1);
    lcd.print("Connected!");
    page = redirect;
    input = "";
    strcpy(config.ssid, tssid);
    EEPROM.put(0, config);
    Serial.println(config.ssid);
    delay(2000);
    lcd.clear();
    delay(100);
    return;
  }

  if (input == "connection error") {
    lcd.clear();
    delay(100);
    lcd.setCursor(7, 1);
    lcd.print("Error!");
    scanning = 1;
    input = "";
    delay(2000);
    lcd.clear();
    delay(100);
  }

  if (scanning == 2) {
    lcd.clear();
    delay(100);
    lcd.setCursor(3, 0);
    lcd.print("Connecting to");
    lcd.setCursor((20 - strlen(tssid)) / 2, 1);
    lcd.print(tssid);
    return;
  }

  if (scanning == 0) {
    Serial1.println("wifi");
    scanning = 1;
  }

  int numItems = size;
  strcpy(wifis[numItems - 1], "Exit");

  if (y != y0 || x != x0) {
    lcd.clear();
    delay(100);
    y = constrain(y, 0, numItems - 1);
  }

  lcd.setCursor(8, 0);
  lcd.print("Wifi");

  int start = numItems - 1 == y ? max(0, y - 2) : max(0, y - 1);
  int end = min(start + 3, numItems);

  for (int i = start; i < end; i++) {
    int row = i - start + 1;
    lcd.setCursor(0, row);

    if (i == y)
      lcd.print("> ");
    else
      lcd.print("  ");

    lcd.print(wifis[i]);
  }

  if (click) {
    click = 0;
    prevent = 1;
    if (y == numItems - 1) {
      page = redirect;
      scanning = 0;
      x = x0 = 0;
      y = y0 = 0;
      if (redirect == 4)
        y = y0 = 2;
      input = "";
      lcd.clear();
    } else {
      scanning = 2;
      strcpy(tssid, wifis[y]);
      Serial1.println(String("wifi ") + tssid + ":chessmaster");
      lcd.clear();
      delay(100);
    }
  }
}

void settings() {
  struct MenuItem {
    const char *label;
    int page;
  };

  const MenuItem settings[] = {{"Level: ", 0}, {"Language: ", 0}, {"Wifi", -3},
                               {"Debug", -4},  {"Credits", 5},    {"Exit", 0}};

  const int numItems = sizeof(settings) / sizeof(settings[0]);

  if (y != y0 || x != x0) {
    lcd.clear();
    delay(100);
    y = constrain(y, 0, numItems - 1);
  }

  lcd.setCursor(6, 0);
  lcd.print("Settings");

  int start = numItems - 1 == y ? max(0, y - 2) : max(0, y - 1);
  int end = min(start + 3, numItems);

  for (int i = start; i < end; i++) {
    int row = i - start + 1;
    lcd.setCursor(0, row);

    if (i == y)
      lcd.print("> ");
    else
      lcd.print("  ");

    lcd.print(settings[i].label);

    if (strcmp(settings[i].label, "Level: ") == 0) {
      if (i == y && x != x0) {
        config.level += (x - x0);
        if (config.level < 1)
          config.level = 15;
        if (config.level > 15)
          config.level = 1;
        x = x0 = 0;
        EEPROM.put(0, config);
      }
      lcd.print("<");
      lcd.print(config.level);
      lcd.print(">");
    }

    if (strcmp(settings[i].label, "Language: ") == 0) {
      if (i == y && x != x0) {
        config.lang += (x - x0);
        if (config.lang < 0)
          config.lang = 0;
        if (config.lang > 0)
          config.lang = 0;
        x = x0 = 0;
        EEPROM.put(0, config);
      }
      lcd.print("<");
      lcd.print(config.lang == 1 ? "it" : "en");
      lcd.print(">");
    }
  }

  if (click) {
    click = 0;
    prevent = 1;
    if (settings[y].page) {
      redirect = 4;
      page = settings[y].page;
      y = x = 0;
      y0 = x0 = 0;
      lcd.clear();
      delay(100);
    } else if (y == numItems - 1) {
      page = redirect = 0;
      y = y0 = 0;
      x = x0 = 3;
      lcd.clear();
      delay(100);
    }
  }
}

void stats() {
  const char *stats[] = {
      "Streak: ", "Wins: ", "Losses: ", "Draws: ", "Games: ", "Exit"};
  const int numItems = sizeof(stats) / sizeof(stats[0]);

  if (y != y0 || x != x0) {
    lcd.clear();
    delay(100);

    y = constrain(y, 0, numItems - 1);
  }

  lcd.setCursor(7, 0);
  lcd.print("Stats");

  int start = numItems - 1 == y ? max(0, y - 2) : max(0, y - 1);
  int end = min(start + 3, numItems);

  for (int i = start; i < end; i++) {
    lcd.setCursor(0, i - start + 1);
    if (i == y)
      lcd.print("> ");
    else
      lcd.print("  ");

    lcd.print(stats[i]);

    if (strcmp(stats[i], "Streak: ") == 0)
      lcd.print(config.streak);
    else if (strcmp(stats[i], "Wins: ") == 0)
      lcd.print(config.wins);
    else if (strcmp(stats[i], "Losses: ") == 0)
      lcd.print(config.losses);
    else if (strcmp(stats[i], "Draws: ") == 0)
      lcd.print(config.draws);
    else if (strcmp(stats[i], "Games: ") == 0)
      lcd.print(config.games);
  }

  if (click && y == numItems - 1) {
    click = 0;
    prevent = 1;
    page = 0;
    y = y0 = 0;
    x = x0 = 2;
    lcd.clear();
    delay(100);
  }
}

void play(int &t, char position[4], int invalid[2]) {
  if (!connected) {
    page = -3;
    lcd.clear();
    delay(200);
    return;
  }

  /*
  String pos = checkEdges(M);
  if (playing == -1 && pos.length() > 0) {
    lcd.setCursor(2, 0);
    lcd.print("Place the pieces");
    lcd.setCursor(4, 1);
    lcd.print("in position!");
    lcd.setCursor(8, 3);
    lcd.print(pos);
    return;
  }*/

  if (playing == -1) {
    Serial1.println("start");
    playing = 0;
  }

  if (x != x0) {
    if (x < 0)
      x = 1;
    if (x > 1)
      x = 0;
  }

  if (playing == 0) {
    t = 0;
    playing = 1;
  }

  if (ddlay(1000))
    timer++;

  unsigned long seconds = (time + (millis() - gap) - timer) / 1000;
  unsigned int sec = seconds % 60;
  unsigned int min = (seconds / 60) % 60;
  unsigned int hour = seconds / 3600;

  char buffer[12] = "00:00";

  if (timer > 1000 && hour < 1000) {
    if (hour > 0) sprintf(buffer, "%02u:%02u:%02u", hour, min, sec);
    else sprintf(buffer, "%02u:%02u", min, sec);
  }

  lcd.setCursor(0, 0);
  if (hour > 0 && timer > 1000 && hour < 1000 && playing == 1) {
    lcd.print("      ");
    lcd.print(buffer);
    lcd.print("      ");
  } else {
    lcd.print("       ");
    lcd.print(buffer);
    lcd.print("        ");
  }

  if (playing == 1) {
    lcd.setCursor(6, 1);
    lcd.print(confirm == 0 ? (t == 0 ? " WHITE  " : " BLACK  ")
                           : "  Exit? ");
  }

  if (playing > 1) {
    confirm = x = 1;
    lcd.setCursor(6, 1);

    if (playing == 2)
      lcd.print("  WIN!  ");
    if (playing == 3)
      lcd.print("  DRAW  ");
    if (playing == 4)
      lcd.print("  LOSE  ");

    lcd.setCursor(4, 3);
    lcd.print("         ");
  }

  if (playing < 2) {
    if (confirm == 0 && playing == 1) {
      lcd.setCursor(6, 3);
      lcd.print("        ");
      if ((invalid[0] == -1 && invalid[1] == -1)) {
        lcd.setCursor(8, 3);
        lcd.print(position);
      } else {
        lcd.setCursor(6, 3);
        lcd.print("Invalid!");
      }
    } else {
      lcd.setCursor(4, 3);
      if (ddlay(500)) {
        lcd.setCursor(4, 3);
        if (x == 1) {
          lcd.print(status % 2 == 0 ? "<Yes>" : "     ");
          lcd.print("   No");
        } else {
          lcd.print("Yes   ");
          lcd.print(status % 2 == 0 ? "<No>" : "     ");
        }
        status = (status + 1) % 2;
      }
    }
  }

  if (click && timer > 1) {
    click = 0;
    prevent = 1;
    lcd.clear();
    delay(100);
    if (confirm == 0) {
      confirm = 1;
      x = x0 = 0;
    } else {
      confirm = 0;
      if (x == 1) {
        x = x0 = stockfish ? 0 : 1;
        y = y0 = 0;
        page = 0;
        timer = 0;
        playing = -1;
        stockfish = false;
        t = 0;
        input = "";
        if (playing == 1)
          Serial1.println("exit");
        config.games += 1;
        EEPROM.put(0, config);
      }
    }
  }
}

void online() {
  if (!connected) {
    page = -3;
    lcd.clear();
    delay(200);
    return;
  }

  if (playing == -1) {
    Serial1.println("start");
    playing = -2;
  }

  if (input.startsWith("code")) {
    String parsed = input.substring(5);
    parsed.toCharArray(code, sizeof(code));
    input = "";
  }

  if (playing == -2 && input == "joined") {
    playing = 0;
    strcpy(code, "");
    input = "";
    lcd.clear();
    delay(100);
  }

  if (playing == -2) {
    lcd.setCursor(1, 0);
    lcd.print("Waiting for Player");
    lcd.setCursor(7, 1);
    lcd.print(code);

    if (ddlay(1000))
      timer++;

    if (ddlay(500)) {
      if (status % 2 == 1) {
        lcd.setCursor(6, 3);
        lcd.print("         ");
      } else {
        lcd.setCursor(7, 3);
        lcd.print("<Exit>");
      }
      status++;

      if (status == 2)
        status = 0;
    }

    if (click && timer > 1) {
      playing = -1;
      strcpy(code, "");
      click = 0;
      prevent = 1;
      timer = 0;
      x = x0 = 1;
      y = y0 = 0;
      page = 0;
      input = "";
      lcd.clear();
      delay(200);
    }
  }

  if (playing == 0) {
    timer = 0;
    page = 1;
  }
}

void home() {
  if (x != x0) {
    lcd.clear();
    delay(100);
    status = 0;

    if (x > 3)
      x = 0;

    if (x < 0)
      x = 3;
  }

  switch (x) {
  case 0:
    lcd.setCursor(7, 1);
    lcd.print("<Play>");
    break;
  case 1:
    lcd.setCursor(6, 1);
    lcd.print("<Online>");
    break;
  case 2:
    lcd.setCursor(6, 1);
    lcd.print("<Stats>");
    break;
  case 3:
    lcd.setCursor(5, 1);
    lcd.print("<Settings>");
    break;
  }

  animation(x + 2);

  if (click) {
    page = x + 1;
    click = 0;
    x = 0;
    x0 = 0;
    y = 0;
    y0 = 0;
    confirm = 0;
    lcd.clear();
  }
}

int lcdloop(int M[cell][cell], int &t, char position[4], int invalid[2]) {
  if (Serial1.available()) {
    input = Serial1.readStringUntil('\n');
    input.trim();
    Serial.println(input);

    if (input.startsWith("wifi ")) {
      size = splitString(input.substring(5), wifis);
      input = "";
    }

    if (input.startsWith("ip ")) {
      ip = input.substring(3);
      input = "";
    }

    if (input.startsWith("time ")) {
      time = input.substring(5).toInt();
      gap = millis();
      input = "";
    }


    if (input == "connected")
      connected = true;

    if (input == "disconnected")
      connected = false;

    if (page == 1) {
      int skip = 0;

      if (input == "stockfish") {
        stockfish = true;
        skip = 1;
      }

      if (input.startsWith("timer ")) {
        timer = input.substring(6).toInt();
        skip = 1;
      }

      if (input == "win") {
        config.wins += 1;
        config.streak += 1;
        playing = 2;
        skip = 1;
        strcpy(position, "");
        t = 0;

        if (config.level < 15 && stockfish)
          config.level += 1;

        EEPROM.put(0, config);
      }

      if (input == "draw") {
        config.draws += 1;
        playing = 3;
        skip = 1;
        strcpy(position, "");
        t = 0;

        EEPROM.put(0, config);
      }

      if (input == "lose") {
        config.losses += 1;
        config.streak = 0;
        playing = 4;
        skip = 1;
        strcpy(position, "");
        t = 0;

        EEPROM.put(0, config);
      }

      if (input == "valid") {
        t = 1;
        skip = 1;
        delay(100);
      }

      if (input == "invalid") {
        char lettera = position[0];
        int colonna = position[1] - '0';

        int rigaIndex = 7 - (colonna - 1);
        int colIndex = lettera - 'a';

        invalid[0] = rigaIndex;
        invalid[1] = colIndex;
        strcpy(position, "");

        t = 0;
        skip = 1;
      }

      if (skip == 0 && !input.startsWith("code") && input != "joined") {
        char piece = input[0];
        input.remove(0, 1);
        strcpy(position, input.c_str());

        char lettera = input.charAt(2);
        int colonna = input.substring(3).toInt();

        int rigaIndex = 7 - (colonna - 1);
        int colIndex = lettera - 'a';

        if (M[rigaIndex][colIndex] == 1) {
          movePiece(input.substring(2), 1);
          eat();
        }
        movePiece(input, 1);
        movePiece(input.substring(2), 0);
        if (piece == 'k') {
          if (input == "e8c8") {
            movePiece("a8", 1);
            movePiece("d8", 0);
          }
          if (input == "e8g8") {
            movePiece("h8", 1);
            movePiece("f8", 0);
          }
        }
        if (piece == 'p') {
          char lstart = input.charAt(0);
          if (lstart != lettera && M[rigaIndex][colIndex] == 0) {
            movePiece(String(lettera) + String(input.charAt(1)), 1);
            eat();
          }
        }

        t = 0;
        invalid[0] = -1;
        invalid[1] = -1;
      }
    }
  }

  if (page == -2) {
    if (config.ssid && strlen(config.ssid) > 0 &&
        (input != "connected" && input != "connection error"))
      return;
    else {
      input = "";
      page = -1;
    }
  }

  if (ddlay(150)) {
    click = !digitalRead(switchPin);
    if (click == 0 && prevent == 1)
      prevent = 0;
    if (click == 1 && prevent == 1)
      click = 0;
  }

  if (ddlay(250)) {
    value1 = treatValue(analogRead(joyPin1));
    value2 = treatValue(analogRead(joyPin2));
    x0 = x;
    y0 = y;

    /*
    Serial.print("y: ");
    Serial.println(value1);
    Serial.print("x: ");
    Serial.println(value2);
    Serial.print("sw: ");
    Serial.println(click);
    */

    if (value2 >= 6)
      x--;

    if (value2 <= 2)
      x++;

    if (value1 >= 6)
      y++;

    if (value1 <= 2)
      y--;
  }

  if (click)
    if (page == -1) {
      page = 0;
      x = 0;
      y = 0;
      click = 0;
      prevent = 1;
      status = 0;
      input = "";
      lcd.clear();
      delay(500);
    }

  if (page == -1 && ddlay(500)) {
    if (status % 2 == 1) {
      lcd.setCursor(6, 3);
      lcd.print("         ");
    } else {
      lcd.setCursor(6, 3);
      lcd.print("<Enter>");
    }
    status++;

    if (status == 2)
      status = 0;
  }

  switch (page) {
  case -8:
    ipaddress();
    break;
  case -7:
    ditch();
    break;
  case -6:
    calibration();
    break;
  case -5:
    squares();
    break;
  case -4:
    debug();
    break;
  case -3:
    wifi();
    break;
  case 0:
    home();
    break;
  case 1:
    play(t, position, invalid);
    break;
  case 2:
    online();
    break;
  case 3:
    stats();
    break;
  case 4:
    settings();
    break;
  case 5:
    credits();
    break;
  }

  delay(5);
  return playing;
}

void lcdbegin() {
  lcd.begin(20, 4);
  randomSeed(analogRead(0));
  lcd.createChar(0, arrowDown);

  pinMode(switchPin, INPUT_PULLUP);
  pinMode(resetlcd, OUTPUT);
  digitalWrite(resetlcd, HIGH);

  digitalWrite(resetlcd, LOW);
  delay(200);
  digitalWrite(resetlcd, HIGH);
  delay(500);

  lcd.setCursor(4, 1);
  lcd.print("CHESS MASTER");
  lcd.setCursor(7, 3);
  lcd.print("v3.4.5");

  delay(2000);

  EEPROM.get(0, config);
  EEPROM.get(addr, S);
  EEPROM.get(addr0, S0);
  Serial.println(config.ssid);

  Serial1.println("wifi");
  Serial1.println("ip");

  if (config.ssid && strlen(config.ssid) > 0) {
    Serial1.println(String("wifi ") + config.ssid + ":chessmaster");
  }
}

void animation(int type) {
  int row = 3;
  int col = 6;

  switch (type) {
  case 0:
    lcd.setCursor(col, row);
    lcd.print(" [0_0] ");
    delay(1000);
    break;
  case 1:
    lcd.setCursor(col, row);
    lcd.print(" [^_^]/");
    delay(1000);

    lcd.setCursor(col, row);
    lcd.print(" [^_^]-");
    delay(1000);
    break;
  case 2:
    if (!ddlay(500))
      break;

    if (status > 2)
      status = 1;

    switch (status) {
    case 0:
      lcd.setCursor(col, row);
      lcd.print(" [0_0]");
      delay(200);
      break;
    case 1:
      lcd.setCursor(col, row);
      lcd.print("\\[^_^] ");
      break;
    case 2:
      lcd.setCursor(col, row);
      lcd.print(" [^_^]\\");
      break;
    }
    status++;
    break;

  case 3:
    if (!ddlay(500))
      break;

    if (status > 2)
      status = 1;

    switch (status) {
    case 0:
      lcd.setCursor(col, row);
      lcd.print(" (O-O)");
      delay(200);
      break;
    case 1:
      lcd.setCursor(col, row);
      lcd.print("\\(^-^) ");
      break;
    case 2:
      lcd.setCursor(col, row);
      lcd.print(" (^-^)\\");
      break;
    }
    status++;
    break;

  case 5:
    if (!ddlay(500))
      break;

    if (status > 1)
      status = 0;

    switch (status) {
    case 0:
      lcd.setCursor(col, row);
      lcd.print("  ***  ");
      break;
    case 1:
      lcd.setCursor(col, row);
      lcd.print("  +-+  ");
      break;
    }
    status++;
    break;

  case 4:
    if (!ddlay(200))
      break;

    lcd.setCursor(7, row);
    unsigned long num = random(10000, 99999);
    lcd.print(num);
    break;
  }
}