#include <LiquidCrystal.h>
#include <EEPROM.h>

#include "utils.h"
#include "servo.h"

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
int addr = sizeof(Config);

//                RS, E, D4, D5, D6, D7
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int joyPin1 = A0;
int joyPin2 = A1;
int switchPin = 13;

int reset = 53;
int value1 = 0; 
int value2 = 0;
int click = 0;
int prevent = 0;

int x0 = 0;
int y0 = 0;
int x = 0;
int y = 0;
int z = 0;
int z0 = 0;

int d = 0;
int r = 0;
int e = 0;
int s = 0;

String input = "";
int page = -2;
int status = 0;
int confirm = 0;
int yes = 0;
int size = -1;

char wifis[10][20];
char tssid[20] = "";
int redirect = 0;
int scanning = 0;
bool connected = false;

long int time[3] = {0, 0, 0};
char code[7] = "";
int playing = -1;
int invalid[2] = {-1, -1};
int turn = 0;

void calibration() {
  lcd.setCursor(4, 0);
  lcd.print("Calibration");

  int ms[4] = {100, 200, 500, 1000};
  if (x != x0 && e) {
    if (x > 4)
      x = 0;
    if (x < 0)
      x = 4;
  }

  if (x < 4) {
    lcd.setCursor(6, 2);
    lcd.print(e ? "<" : " ");
    lcd.print(ms[x]);
    if (e) lcd.print(">");
    lcd.print(" ms   ");
  } else {
    lcd.setCursor(7, 2);
    lcd.print("<Exit>  ");
  }

  if (y != y0 && x < 4) {
    y = constrain(y, -1, 1);

    if (y == 1) {
      down(ms[x]);
    }
    if (y == -1) {
      up(ms[x]);
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
      y = x = 0;
      y0 = x0 = 0;
      lcd.clear();
      delay(100);
    }
  }
}

void squares() {
  struct MenuItem {
    const char* label;
    int page;
  };

  const MenuItem settings[] = {
    {"Square: ", 0},
    {"Turn: ", 0},
    {"Move: ", 0},
    {"Down: ", 0},
    {"Exit", 0}
  };

  const int numItems = sizeof(settings) / sizeof(settings[0]);

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

  if (S[7 - z][z0].turn < 0)
    S[7 - z][z0].turn = 90;
  if (S[7 - z][z0].move < 0)
    S[7 - z][z0].move = 0;
  if (S[7 - z][z0].down < 0)
    S[7 - z][z0].down = 0; 
  
  for (int i = start; i < end; i++) {
    int row = i - start + 1;
    lcd.setCursor(0, row);

    if (i == y) lcd.print("> ");
    else lcd.print("  ");

    lcd.print(settings[i].label);
    if (strcmp(settings[i].label, "Square: ") == 0) {
      if (i == y && x != x0) {
        if (s == 1)
          z0 += (x - x0);
        else
          z += (x - x0);
        
        if (z < 0) {
          z = 7;
          if (s == 0) z0--;
        }
        if (z > 7) {
          z = 0;
          if (s == 0) z0++;
        }
        if (z0 < 0) z0 = 7;
        if (z0 > 7) z0 = 0;
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

    if (strcmp(settings[i].label, "Turn: ") == 0) {
      if (i == y && x != x0) {
        S[7 - z][z0].turn += range[r] * (x - x0);
        if (S[7 - z][z0].turn < 0) S[7 - z][z0].turn = 180;
        if (S[7 - z][z0].turn > 180) S[7 - z][z0].turn = 0;
        x = x0 = 0;
        EEPROM.put(addr, S);
      }

      if (i == y)
        moveH(S[7 - z][z0].turn);

      lcd.print("<");
      lcd.print(S[7 - z][z0].turn);
      lcd.print(">");
    }

    if (strcmp(settings[i].label, "Move: ") == 0) {
      if (i == y && x != x0) {
        S[7 - z][z0].move += range[r] * (x - x0);
        if (S[7 - z][z0].move < 0) S[7 - z][z0].move = 180;
        if (S[7 - z][z0].move > 180) S[7 - z][z0].move = 0;
        x = x0 = 0;
        EEPROM.put(addr, S);
      }

      if (i == y)
        moveV(S[7 - z][z0].move);

      lcd.print("<");
      lcd.print(S[7 - z][z0].move);
      lcd.print(">");
    }

    if (strcmp(settings[i].label, "Down: ") == 0) {
      if (i == y && x != x0) {
        S[7 - z][z0].down += range[r] * (x - x0);
        S[7 - z][z0].down = constrain(S[7 - z][z0].down, 0, 150);
        x = x0 = 0;
        EEPROM.put(addr, S);
      }

      if (i == y) {
        if (S[7 - z][z0].down > d)
          down((S[7 - z][z0].down - d) * 100);
        else
          up((d - S[7 - z][z0].down) * 100);
        d = S[7 - z][z0].down;
      }

      lcd.print("<");
      lcd.print(S[7 - z][z0].down * 100);
      lcd.print(">");
    }
  }

  if (click) {
    click = 0;
    prevent = 1;
    if (settings[y].page) {
      redirect = -4;
      page = settings[y].page;
      y = x = 0;
      y0 = x0 = 0;
      lcd.clear();
      delay(100);
    } else if (y == numItems - 1) {
      page = redirect;
      y = x = 0;
      y0 = x0 = 0;
      moveH(90);
      moveV(0);
      up(d * 100);
      d = 0;
      lcd.clear();
      delay(100);
    } else if (y == 0) {
      s++;
      if (s > 2) s = 0;
    } else {
      r++;
      if (r > 3) r = 0;
    }
  }
}

void debug() {
  struct MenuItem {
    const char* label;
    int page;
  };

  const MenuItem settings[] = {
    {"Squares", -6},
    {"Calibration", -5},
    {"Exit", 0}
  };

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

    if (i == y) lcd.print("> ");
    else lcd.print("  ");

    lcd.print(settings[i].label);
  }

  if (click) {
    click = 0;
    prevent = 1;
    if (settings[y].page) {
      redirect = -4;
      page = settings[y].page;
      y = x = 0;
      y0 = x0 = 0;
      lcd.clear();
      delay(100);
    } else if (y == numItems - 1) {
      page = redirect = 4;
      y = x = 0;
      y0 = x0 = 0;
      lcd.clear();
      delay(100);
    }
  }
}

void credits() {
  const char* credits[] = {
    "Giovanni Montagna",
    "Matteo Geusa",
    "Samuele Putignani",
    "Camilla Torresin",
    "Lorenzo Afrune",
    "Alessandro Meraglia",
    "Paola Candido",
    "Giulia Levanto",
    ""
  };

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
    y = x = 0;
    y0 = x0 = 0;
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
    lcd.setCursor(7, 1);
    lcd.print("Error!");
    scanning = 1;
    input = "";
    delay(2000);
    lcd.clear();
    delay(100);
  }

  if (scanning == 2) {
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

    if (i == y) lcd.print("> ");
    else lcd.print("  ");

    lcd.print(wifis[i]);
  }

  if (click) {
    click = 0;
    prevent = 1;
    if (y == numItems - 1) {
      page = redirect;
      scanning = 0;
      y = x = 0;
      y0 = x0 = 0;
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
    const char* label;
    int page;
  };

  const MenuItem settings[] = {
    {"Level: ", 0},
    {"Language: ", 0},
    {"Wifi", -3},
    {"Debug", -4},
    {"Credits", 5},
    {"Exit", 0}
  };

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

    if (i == y) lcd.print("> ");
    else lcd.print("  ");

    lcd.print(settings[i].label);

    if (strcmp(settings[i].label, "Level: ") == 0) {
      if (i == y && x != x0) {
        config.level += (x - x0);
        if (config.level < 1) config.level = 15;
        if (config.level > 15) config.level = 1;
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
        if (config.lang < 0) config.lang = 0;
        if (config.lang > 0) config.lang = 0;
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
      y = x = 0;
      y0 = x0 = 0;
      lcd.clear();
      delay(100);
    }
  }
}

void stats() {
  const char* stats[] = {
    "Streak: ",
    "Wins: ",
    "Losses: ",
    "Draws: ",
    "Games: ",
    "Exit"
  };
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
    if (i == y) lcd.print("> ");
    else lcd.print("  ");

    lcd.print(stats[i]);

    if (strcmp(stats[i], "Streak: ") == 0) lcd.print(config.streak);
    else if (strcmp(stats[i], "Wins: ") == 0) lcd.print(config.wins);
    else if (strcmp(stats[i], "Losses: ") == 0) lcd.print(config.losses);
    else if (strcmp(stats[i], "Draws: ") == 0) lcd.print(config.draws);
    else if (strcmp(stats[i], "Games: ") == 0) lcd.print(config.games);
  }

  if (click && y == numItems - 1) {
    click = 0;
    prevent = 1;
    page = 0;
    y = x = 0;
    y0 = x0 = 0;
    lcd.clear();
  }
}

void play(int M[cell][cell], char position[4]) {
  if (!connected) {
    page = -3;
    lcd.clear();
    delay(200);
    return;
  }

  /*
  String pos = checkEdges(M);
  if (playing == 0 && pos.length() > 0) {
    lcd.setCursor(2, 0);
    lcd.print("Place the pieces");
    lcd.setCursor(4, 1);
    lcd.print("in position!");
    lcd.setCursor(8, 3);
    lcd.print(pos);
    return;
  }*/

  if (ddlay(1000)) {
    time[1]++;
    if (time[1] > 59) {
      time[1] = 0;
      time[0]++;
    }
  }

  if (x != x0) {
    if (x < 0)
      x = 1;
    if (x > 1)
      x = 0;
  }

  if (playing == 0)
    playing = 1;

  lcd.setCursor(7, 0);
  if (time[0] < 10) lcd.print("0");
  lcd.print(time[0]);
  lcd.print(":");
  if (time[1] < 10) lcd.print("0");
  lcd.print(time[1]);

  if (playing == 1) {
    lcd.setCursor(6, 1);
    lcd.print(confirm == 0 ? (turn == 0 ? " WHITE  " : " BLACK  ") : "  Exit? ");
  }

  if (playing > 1) {
    confirm = x = 1;
    lcd.setCursor(8, 1);

    if (playing == 2)
      lcd.print("WIN!");
    if (playing == 3)
      lcd.print("LOSE");
    if (playing == 4)
      lcd.print("DRAW");
  }

  if (confirm == 0) {
    if (invalid[0] != -1 && invalid[1] != -1 && M[invalid[0]][invalid[1]] == 1) {
      invalid[0] = -1;
      invalid[1] = -1;
    }

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

  if (click && (time[0] > 0 || time[1] > 1)) {
    click = 0;
    prevent = 1;
    lcd.clear();
    delay(100);
    if (confirm == 0) {
      confirm = 1;
      x = x0 = 0;
    } else {
      if (x == 1) {
        x = x0 = 0;
        page = 0;
        time[0] = 0;
        time[1] = 0;
        playing = -1;
        input = "";
        if (playing == 1)
          Serial1.println("exit");
        config.games += 1;
        EEPROM.put(0, config);
      } else {
        confirm = 0;
      }
    }
  }
}

void online(int M[cell][cell]) {
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
      time[2]++;

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

    if (click && time[2] > 1) {
      playing = -1;
      strcpy(code, "");
      click = 0;
      prevent = 1;
      x = x0 = 0;
      y = y0 = 0;
      time[2] = 0;
      page = 0;
      input = "";
      lcd.clear();
      delay(200);
    }
  }

  if (playing == 0)
    page = 1;
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

    switch(x){
      case 0: 
        lcd.setCursor(7,1);
        lcd.print("<Play>");
        break;
      case 1: 
        lcd.setCursor(6,1);
        lcd.print("<Online>");
        break;
      case 2: 
        lcd.setCursor(6,1);
        lcd.print("<Stats>");
        break;
      case 3: 
        lcd.setCursor(5,1);
        lcd.print("<Settings>");
        break;
    }

    animation(x + 2);

    if (click) {
      page = x + 1;
      x = 0;
      x0 = 0;
      y = 0;
      y0 = 0;
      lcd.clear();
    }
}

int lcdloop(int M[cell][cell], char position[4]) {
  if (Serial1.available()) {
    input = Serial1.readStringUntil('\n');
    input.trim();
    Serial.println(input);

    if (input.startsWith("wifi ")) {
      size = splitString(input.substring(5), wifis);
      input = "";
    }

    if (input == "connected")
      connected = true;

    if (input == "disconnected")
      connected = false;

    if (page == 1) {
      int skip = 0;

      if (input == "win") {
        config.wins += 1;
        config.streak += 1;

        if (config.level < 15)
          config.level += 1;

        EEPROM.put(0, config);
        skip = 1;
      }

      if (input == "draw") {
        config.draws += 1;

        EEPROM.put(0, config);
        skip = 1;
      }

      if (input == "lose") {
        config.losses += 1;
        config.streak = 0;

        EEPROM.put(0, config);
        skip = 1;
      }

      if (input == "valid") {
        turn = 1;
        skip = 1;
      }

      if (input == "invalid") {
        char lettera = position[0];
        int colonna = position[1] - '0';

        int rigaIndex = 7 - (colonna - 1);
        int colIndex = lettera - 'a';

        invalid[0] = rigaIndex;
        invalid[1] = colIndex;
        strcpy(position, "");

        turn = 0;
        skip = 1;
      }

      if (skip == 0 && !input.startsWith("code") && input != "joined") {
        strcpy(position, input.c_str());

        char lettera = input.charAt(2);
        int colonna = input.substring(3).toInt();

        int rigaIndex = 7 - (colonna - 1);
        int colIndex = lettera - 'a';

        if (M[rigaIndex][colIndex] == 1) {
          move(input.substring(2), 1);
          eat();
        }
        move(input, 1);
        move(input.substring(2), 0);
        moveV(0);
        moveH(90);
        turn = 0;
        invalid[0] = -1;
        invalid[1] = -1;
      }
    }
  }


  if (page == -2) {
    if (config.ssid && strlen(config.ssid) > 0 && (input != "connected" && input != "connection error")) return;
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

    switch(page){
      case -6:
        squares();
        break;
      case -5:
        calibration();
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
        play(M, position);
        break;
      case 2:
        online(M);
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
  pinMode(reset, OUTPUT);
  digitalWrite(reset, HIGH);

  digitalWrite(reset, LOW);
  delay(200);
  digitalWrite(reset, HIGH);
  delay(500);

  lcd.setCursor(4, 1);
  lcd.print("CHESS MASTER");
  lcd.setCursor(7, 3);
  lcd.print("v1.0.0");

  delay(2000);

  EEPROM.get(0, config);
  EEPROM.get(addr, S);
  Serial.println(config.ssid);

  Serial1.println("wifi");
  if (config.ssid && strlen(config.ssid) > 0) {
    Serial1.println(String("wifi ") + config.ssid + ":chessmaster");
  }
}

void animation(int type) {
    int row = 3;
    int col = 6;

  switch(type) {
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
      if (!ddlay(500)) break;

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
      if (!ddlay(500)) break;

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
        if (!ddlay(500)) break;

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
        if (!ddlay(200)) break;

        lcd.setCursor(7, row);
        unsigned long num = random(10000, 99999);
        lcd.print(num);
        break;
  }
}