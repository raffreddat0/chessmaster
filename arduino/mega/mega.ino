#include "menu.h"

int rows[] = {51, 47, 43, 39, 35, 31, 27, 23};
int cols[] = {53, 49, 45, 41, 37, 33, 29, 25};
int M[cell][cell], M0[cell][cell];
char position[4] = "";
int moved[2] = {0, 0};

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  lcdbegin();
  serbegin();

  for (int i = 0; i < cell; i++) {
    pinMode(cols[i], OUTPUT);
    pinMode(rows[i], INPUT_PULLUP);
  }

  for (int row = 0; row < cell; row++) {
    for (int col = 0; col < cell; col++) {
      for (int i = 0; i < cell; i++)
        digitalWrite(cols[i], LOW);

      digitalWrite(cols[col], HIGH);
      M[row][col] = M0[row][col] = digitalRead(rows[row]);
    }
  }
}

void loop() {
  int playing = lcdloop(M);
  serloop();
  
  if (ddlay(500)) {
    Serial.print("\n\n\n"); 
    for (int row = 0; row < cell; row++) {    
      for (int col = 0; col < cell; col++) {
        for (int i = 0; i < cell; i++)
           digitalWrite(cols[i], LOW);

        digitalWrite(cols[col], HIGH);
        M[row][col] = digitalRead(rows[row]);
        if (M[row][col] != M0[row][col] && M0[row][col] == 1) {
          position[0] = char('a' + col);
          position[1] = char('1' + 7 - row);
          moved[0] = 1;
          //moved[1] = 0;
        }

        if (M[row][col] != M0[row][col] && M0[row][col] == 0) {
          position[2] = char('a' + col);
          position[3] = char('1' + 7 - row);
          moved[1] = 1;
        }

        M0[row][col] = M[row][col];
        Serial.print(M[row][col]);
      }
      Serial.print("\n");
    }
  }

  if (moved[0] == 1 && moved[1] == 1) {
    if (playing == 1) {
      //Serial.println(String(position));
      Serial1.println(String("move ") + String(position));
    }
    moved[0] = 0;
    moved[1] = 0;
  }
}