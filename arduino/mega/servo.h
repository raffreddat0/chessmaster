#include <Servo.h>

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

int h0 = 140, v0 = 60, d0 = 10000;
int prev = 0, prev1 = 90, prev2 = 0;

struct Square {
  int turn;
  int move;
  int down;
};

extern Square S[8][8] = {};

void serbegin() {
  Serial.begin(9600);

  pinMode(6, LOW);

  servo1.detach();
  servo2.detach();
  servo3.detach();
  servo4.detach();

  servo1.attach(5);
  servo1.write(90);

  servo3.attach(3);
  servo4.attach(2);
  servo3.write(0);
  servo4.write(180);

  delay(1000);
}

void moveV(int degree) {
  servo3.attach(3);
  servo4.attach(2);
  if (degree > prev) {
    for (int angolo = prev; angolo <= degree; angolo++) {
      servo3.write(angolo);
      servo4.write(180 - angolo);
      delay(10);
    }
  } else {
    for (int angolo = prev; angolo >= degree; angolo--) {
      servo3.write(angolo);
      servo4.write(180 - angolo);
      delay(10);
    }
  }
  prev = degree;
}

void moveH(int degree) {
  servo1.attach(5);
  if (degree > prev1)
    for (int angolo = prev1; angolo <= degree; angolo++) {
      servo1.write(angolo);
      delay(10);
    }
  else
    for (int angolo = prev1; angolo >= degree; angolo--) {
      servo1.write(angolo);
      delay(10);
    }
  prev1 = degree;
}

void up(int ms) {
  servo2.attach(4);
  servo2.writeMicroseconds(1417);
  delay(ms);
  servo2.detach();
}

void down(int ms) {
  servo2.attach(4);
  servo2.writeMicroseconds(1528);
  delay(ms);
  servo2.detach();
}

void move(String position, int magnet = 0) {
  char lettera = position.charAt(0);
  int colonna = position.substring(1).toInt();

  int rigaIndex = 7 - (colonna - 1);
  int colIndex = lettera - 'a';

  if (rigaIndex >= 0 && rigaIndex < 8 && colIndex >= 0 && colIndex < 8) {
    int h = S[rigaIndex][colIndex].turn;
    int v = S[rigaIndex][colIndex].move;
    int d = S[rigaIndex][colIndex].down;

    Serial.print("Giro: "); Serial.println(h);
    Serial.print("Muovo: "); Serial.println(v);
    Serial.print("Discesa: "); Serial.println(d);

    moveH(h < 0 ? 0 : h);
    moveV(v < 0 ? 0 : v);
    down(d < 0 ? 0 : d*100);
    pinMode(6, magnet);
    up(d < 0 ? 0 : d*100);
  }
}

void eat() {
  moveH(h0);
  moveV(v0);
  down(d0);
  pinMode(6, 0);
  up(d0);
}

void serloop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    Serial.println(input);

    if (input == "on") pinMode(6, HIGH);
    else if (input == "off") pinMode(6, LOW);
    else if (input.startsWith("eat")) {
      eat();
    } else if (input.startsWith("up")) {
      int delayTime = input.substring(3).toInt();
      up(delayTime);
    } else if (input.startsWith("down")) {
      int delayTime = input.substring(4).toInt();
      down(delayTime);
    } else if (input.startsWith("turn")) {
      int degree = input.substring(5).toInt();
      moveH(degree);
    } else if (input.startsWith("move")) {
      int degree = input.substring(5).toInt();
      moveV(degree);
    } else if (input.startsWith("test")) {
      int degree = input.substring(5).toInt();
      servo2.attach(4);
      servo2.write(degree);
    } else {
      move(input, 1);
      if (input.length() > 2) {
        move(input.substring(2), 0);
        moveV(0);
        moveH(90);
      }
    }
  }
}