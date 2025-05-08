#include "menu.h"

const int cell = 8;
int rows[] = {52,50,48,46,44,42,40,38};
int cols[] = {22,24,26,28,30,32,34,36};
int M[cell][cell];

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  lcdbegin();

  for (int i = 0; i < cell; i++) {
        pinMode(rows[i], OUTPUT);
        pinMode(cols[i], INPUT_PULLUP);
    }
}

void loop() {
  lcdloop();

  if (ddlay(2000)) {
    Serial.print("\n\n\n"); 
    for (int row = 0; row < cell; row++) {
        for (int i = 0; i < cell; i++)
           digitalWrite(rows[i], LOW);
    
        digitalWrite(rows[row], HIGH);

        for (int col = 0; col < cell; col++) {

            M[row][col] = digitalRead(cols[col]);
            Serial.print(M[row][col]);
      
        }
    
        Serial.print("\n");
    }
  }
}

/*
void settings() {
  if (y != y0 || x != x0) {
    lcd.clear();
    delay(100);

    if (y > 4)
      y = 4;

    if (y < 0)
      y = 0;

    if (y == 0)
      menu = 0;

    if (menu == 0 && y == 3 || (menu == 2 && y == 1))
      menu = 1;
    
    if (y == 4)
      menu = 2;
  }
  
  lcd.setCursor(6, 0);
  lcd.print("Settings");

  char settings[][21] = {
    "Level: ",
    "Language: ",
    "Wifi",
    "Credits",
    "Exit"
  };

  lcd.setCursor(0, 1);
  if (y == 0 || (menu == 2 && y == 2) || (menu == 1 && y == 1))
    lcd.print("> ");
  lcd.print(settings[menu]);

  if (menu == 0 && y == 0) {
    if (x != x0) {
      config.level = config.level + (x - x0);
      if (config.level < 1)
        config.level = 20;
      if (config.level > 20)
        config.level = 1;
      x = 0;
      x0 = 0;

      EEPROM.put(0, config);
    }

    lcd.print("<");
    lcd.print(config.level);
    lcd.print(">");
  } else {
    if (menu == 1)
      lcd.print(config.lang == 1 ? "it" : "en");
    if (menu == 0)
      lcd.print(config.level);
  }

  lcd.setCursor(0, 2);
  if ((menu == 0 && y == 1) || (menu == 2 && y == 3) || (menu == 1 && y == 2))
    lcd.print("> ");
  lcd.print(settings[menu + 1]);

  if ((menu == 0 && y == 1) || (menu == 1 && y == 0)) {
    if (x != x0) {
      config.lang = config.lang + (x - x0);
      if (config.lang < 0)
        config.lang = 1;
      if (config.lang > 1)
        config.lang = 0;
      x = 0;
      x0 = 0;

      EEPROM.put(0, config);
    }

    lcd.print("<");
    lcd.print(config.lang == 1 ? "it" : "en");
    lcd.print(">");
  } else if (menu == 0)
      lcd.print(config.lang == 1 ? "it" : "en");

  lcd.setCursor(0, 3);
  if ((menu == 0 && y == 2) || (menu == 1 && y == 3) || y == 4)
    lcd.print("> ");
  lcd.print(settings[menu + 2]);

  if (menu == 0 || menu == 1) {
    lcd.setCursor(19, 3);
    lcd.write(byte(0));
  }

  if (menu == 1 || menu == 2) {
    lcd.setCursor(19, 1);
    lcd.print("^");
  }

  if (click) {
    if (menu == 2 && y == 4) {
      page = -1;
      y = 0;
      x = 0;
      menu = 0;
    }
  }

}

void stats() {
  if (y != y0 || x != x0) {
    lcd.clear();
    delay(100);

    if (y > 4)
      y = 4;

    if (y < 0)
      y = 0;

    if (y == 0)
      menu = 0;

    if (menu == 0 && y == 3 || (menu == 2 && y == 1))
      menu = 1;
    
    if (y == 4)
      menu = 2;
  }
  
  lcd.setCursor(7, 0);
  lcd.print("Stats");

  char stats[][21] = {
    "Streak: ",
    "Wins: ",
    "Losses: ",
    "Games: ",
    "Exit"
  };

  lcd.setCursor(0, 1);
  if (y == 0 || (menu == 2 && y == 2) || (menu == 1 && y == 1))
    lcd.print("> ");
  lcd.print(stats[menu]);

  if (menu == 1)
    lcd.print(config.wins);
  if (menu == 0)
    lcd.print(config.streak);
  if (menu == 2)
    lcd.print(config.losses);

  lcd.setCursor(0, 2);
  if ((menu == 0 && y == 1) || (menu == 2 && y == 3) || (menu == 1 && y == 2))
    lcd.print("> ");
  lcd.print(stats[menu + 1]);

  if (menu == 0)
    lcd.print(config.wins);
  if (menu == 1)
    lcd.print(config.losses);
  if (menu == 2)
    lcd.print(config.games);

  lcd.setCursor(0, 3);
  if ((menu == 0 && y == 2) || (menu == 1 && y == 3) || y == 4)
    lcd.print("> ");
  lcd.print(stats[menu + 2]);

  if (menu == 0)
    lcd.print(config.losses);
  if (menu == 1)
    lcd.print(config.games);

  if (menu == 0 || menu == 1) {
    lcd.setCursor(19, 3);
    lcd.write(byte(0));
  }

  if (menu == 1 || menu == 2) {
    lcd.setCursor(19, 1);
    lcd.print("^");
  }

  if (click) {
    if (menu == 2 && y == 4) {
      page = -1;
      y = 0;
      x = 0;
      menu = 0;
    }
  }

}
*/