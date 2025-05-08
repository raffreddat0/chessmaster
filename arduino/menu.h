#include <LiquidCrystal.h>
#include <EEPROM.h>

int status = 0;
void animation(int type);

struct Config {
  int level;
  int lang;
  char ssid[32];
  int streak;
  int wins;
  int losses;
  int games;
};

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int joyPin1 = A0;
int joyPin2 = A1;
int switchPin = 2;
int value1 = 0; 
int value2 = 0;
int click = 0;
int row = 0;
int col = 0;
int d = 1;
int x0 = 0;
int y0 = 0;
int x = 0;
int y = 0;
int page = -2;
Config config;
int menu = 0;
long int time[3] = {0, 0, 0}; 
int turn = 0;
int confirm = 0;
int yes = 0;
String input = "";
int playing = -1;
char wifis[10][20];
int redirect = 0;
int scanning = 0;
int size = -1;

byte frecciaGiu[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b10001,
  0b01010,
  0b00100,
  0b00000
};

void splitString(String input, char output[][20], char sep = ',') {
  int start = 0;
  size = 0;

  while (size < 10) {
    int idx = input.indexOf(sep, start);
    if (idx == -1) idx = input.length();

    String temp = input.substring(start, idx);
    temp.trim();
    
    bool isDuplicate = false;
    for (int i = 0; i < size; i++) {
      if (temp.equals(output[i])) {
        isDuplicate = true;
        break;
      }
    }

    if (!isDuplicate) {
      temp.toCharArray(output[size], 20);
      size++;
    }

    if (idx == input.length()) break;
    start = idx + 1;
  }
}

int treatValue(int data) {
  return (data * 9 / 1024);
}

const int MAX_KEYS = 20;
unsigned long intervals[MAX_KEYS];
unsigned long lastTimes[MAX_KEYS];

bool ddlay(unsigned long interval) {
  unsigned long now = millis();

  for (int i = 0; i < MAX_KEYS; i++) {
    if (intervals[i] == interval) {
      if (now - lastTimes[i] >= interval) {
        lastTimes[i] = now;
        return true;
      } else {
        return false;
      }
    }
  }

  for (int i = 0; i < MAX_KEYS; i++) {
    if (intervals[i] == 0) {
      intervals[i] = interval;
      lastTimes[i] = now;
      return true;
    }
  }
  return false;
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
    page = 4;
    y = x = 0;
    y0 = x0 = 0;
    lcd.clear();
  }
}

void wifi() {
  if (input.startsWith("connected")) {
    lcd.clear();
    delay(100);
    lcd.setCursor(5, 1);
    lcd.print("Connected!");
    page = redirect;
    input = "";
    strcpy(config.ssid, wifis[scanning - 2]);
    EEPROM.put(0, config);
    Serial.println(config.ssid);
    delay(2000);
    lcd.clear();
    delay(100);
    return;
  }
  
  if (input.startsWith("error")) {
    lcd.setCursor(7, 1);
    lcd.print("Error!");
    scanning = 1;
    input = "";
    delay(2000);
    lcd.clear();
    delay(100);
  }

  if (scanning > 1) {
    lcd.setCursor(3, 0);
    lcd.print("Connecting to");
    lcd.setCursor((20 - strlen(wifis[scanning - 2])) / 2, 1);
    lcd.print(wifis[scanning - 2]);
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
    if (y == numItems - 1) {
      page = redirect;
      scanning = 0;
      y = x = 0;
      y0 = x0 = 0;
      input = "";
      lcd.clear();
    } else {
      Serial.println(y);
      scanning = y + 2;
      Serial1.println(String("wifi ") + wifis[y] + ":chessmaster");
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
        if (config.level < 1) config.level = 20;
        if (config.level > 20) config.level = 1;
        x = x0 = 0;
        EEPROM.put(0, config);
      }
      lcd.print("<");
      lcd.print(config.level);
      lcd.print(">");
    }
    else if (strcmp(settings[i].label, "Language: ") == 0) {
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
    }
  }
}

void stats() {
  const char* stats[] = {
    "Streak: ",
    "Wins: ",
    "Losses: ",
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
    else if (strcmp(stats[i], "Games: ") == 0) lcd.print(config.games);
  }

  if (click && y == numItems - 1) {
    click = 0;
    page = 0;
    y = x = 0;
    y0 = x0 = 0;
    lcd.clear();
  }
}

void play() {
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

  playing = 1;

  lcd.setCursor(7, 0);
  if (time[0] < 10) lcd.print("0");
  lcd.print(time[0]);
  lcd.print(":");
  if (time[1] < 10) lcd.print("0");
  lcd.print(time[1]);

  lcd.setCursor(6, 1);
  lcd.print(confirm == 0 ? (turn == 0 ? " WHITE  " : " BLACK  ") : "Confirm?");

  if (confirm == 0) {
    if (ddlay(500)) {
      lcd.setCursor(6, 3);
      if (status % 2 == 0)
        lcd.print("<Exit>");
      else
        lcd.print("           ");
      status = (status + 1) % 2;
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
        playing = 0;
        input = "";
      } else {
        confirm = 0;
      }
    }
  }
}

void online() {
  if (playing == -1) {
    Serial1.println("start");
    playing = 0;
  }

  if (playing = 0 && !input.equals("")) {
    lcd.setCursor(1, 1);
    lcd.print("Waiting for Player");
    lcd.setCursor(7, 2);
    lcd.print(input);

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
      lcd.clear();
      delay(100);
      click = 0;
      x = x0 = 0;
      y = y0 = 0;
      time[3] = 0;
      page = 0;
      input = "";
    }
  }

  if (playing == 1)
    play();
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

void lcdloop() {
  if (Serial1.available()) {
    input = Serial1.readStringUntil('\n');
    Serial.println(input);

    if (input.startsWith("wifi")) {
      splitString(input.substring(5), wifis);
      input = "";
    }
  }

  if (page == -2 ) {
    if (config.ssid && strlen(config.ssid) > 0 && (!input.startsWith("connected") && !input.startsWith("error")))
      return;
    else {
      input = "";
      page = -1;
    }
  }

  if (ddlay(150))
    click = !digitalRead(switchPin);

  if (ddlay(250)) { 
    value1 = treatValue(analogRead(joyPin1));         
    value2 = treatValue(analogRead(joyPin2));
    x0 = x;
    y0 = y;

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
      case -3:
        wifi();
        break;
      case 0: 
        home();
        break;
      case 1:
        play();
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
}

void lcdbegin() {
  lcd.begin(20, 4);
  randomSeed(analogRead(0));
  lcd.createChar(0, frecciaGiu);

  pinMode(switchPin, INPUT_PULLUP);
  //lcd.blink();
  lcd.setCursor(4, 1);
  lcd.print("CHESS MASTER");

  lcd.setCursor(7, 3);
  lcd.print("v0.1.0");

  delay(2000);

  //config = {1, 0, "", 0, 0, 0, 0};
  EEPROM.get(0, config);
  Serial.println(config.ssid);

  if (config.ssid && strlen(config.ssid) > 0) {
    Serial1.println(String("wifi ") + config.ssid + ":chessmaster");
  }

  Serial1.println("wifi");
}

void animation(int type) {
    int row = 3;
    int col = 6;

    //Serial.println(type);

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