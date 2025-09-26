#include <Servo.h>

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

int h0 = 140, v0 = 60, d0 = 10000;
int prev = 0, prev1 = 90, prev2 = 0;

const int angleH[8][8] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

const int angleV[8][8] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

const int values[8][8] = {
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};

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
    //servo3.write(degree);
    //servo4.write(180 - degree);
    for (int angolo = prev; angolo <= degree; angolo++) {
      servo3.write(angolo);
      servo4.write(180 - angolo);
      delay(30);
    }
  } else {
    //servo3.write(degree);
    //servo4.write(180 - degree);
    for (int angolo = prev; angolo >= degree; angolo--) {
      servo3.write(angolo);
      servo4.write(180 - angolo);
      delay(30);
    }
  }
  prev = degree;
}

void moveH(int degree) {
  servo1.attach(5);
  if (degree > prev1)
    for (int angolo = prev1; angolo <= degree; angolo++) {
      servo1.write(angolo);
      delay(30);
    }
  else
    for (int angolo = prev1; angolo >= degree; angolo--) {
      servo1.write(angolo);
      delay(30);
    }
  prev1 = degree;
}

void up(int ms) {
  servo2.attach(4);
  servo2.writeMicroseconds(40);
  delay(ms);
  servo2.detach();
}

void down(int ms) {
  servo2.attach(4);
  servo2.writeMicroseconds(3040);
  delay(ms);
  servo2.detach();
}

void move(String position, int magnet = 0) {
  char lettera = position.charAt(0);
  int colonna = position.substring(1).toInt();

  int rigaIndex = 7 - (colonna - 1);
  int colIndex = lettera - 'a';

  if (rigaIndex >= 0 && rigaIndex < 8 && colIndex >= 0 && colIndex < 8) {
    int h = angleH[rigaIndex][colIndex];
    int v = angleV[rigaIndex][colIndex];
    int d = values[rigaIndex][colIndex];

    Serial.print("Giro: "); Serial.println(h);
    Serial.print("Muovo: "); Serial.println(v);
    Serial.print("Discesa: "); Serial.println(d);

    moveH(h);
    moveV(v);
    down(d);
    pinMode(6, magnet);
    up(d + d / 12);
  }
}

void eat() {
  moveH(h0);
  moveV(v0);
  down(d0);
  pinMode(6, 0);
  up(d0 + d0 / 12);
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
    } else if (input.startsWith("su")) {
      int delayTime = input.substring(3).toInt();
      up(delayTime);
    } else if (input.startsWith("giu")) {
      int delayTime = input.substring(4).toInt();
      down(delayTime);
    } else if (input.startsWith("gira")) {
      int degree = input.substring(5).toInt();
      moveH(degree);
    } else if (input.startsWith("move")) {
      int degree = input.substring(5).toInt();
      moveV(degree);
    } else {
      move(input, 1);
      moveV(45);
      moveH(90);
      if (input.length() > 2) {
        move(input.substring(2), 0);
        moveV(0);
        moveH(90);
      }
      /*
      char lettera = input.charAt(0);
      int colonna = input.substring(1).toInt();

      int rigaIndex = 7 - (colonna - 1);
      int colIndex = lettera - 'a';

      if (rigaIndex >= 0 && rigaIndex < 8 && colIndex >= 0 && colIndex < 8) {
        int h = angleH[rigaIndex][colIndex];
        int v = angleV[rigaIndex][colIndex];
        int d = values[rigaIndex][colIndex];

        Serial.print("Giro: "); Serial.println(h);
        Serial.print("Muovo: "); Serial.println(v);
        Serial.print("Discesa: "); Serial.println(d);

        servo1.attach(5);
        if (h > prev1)
          for (int angolo = prev1; angolo <= h; angolo++) {
            servo1.write(angolo);
            delay(15);
          }
        else
          for (int angolo = prev1; angolo >= h; angolo--) {
            servo1.write(angolo);
            delay(15);
          }
        prev1 = h;

        servo3.attach(3);
        servo4.attach(2);
        if (v > prev)
          for (int angolo = prev; angolo <= v; angolo++) {
            servo3.write(angolo);
            servo4.write(180 - angolo);
            delay(15);
          }
        else
          for (int angolo = prev; angolo >= v; angolo--) {
            servo3.write(angolo);
            servo4.write(180 - angolo);
            delay(15);
          }
        prev = v;

        servo2.attach(4);
        servo2.writeMicroseconds(3040);
        delay(d);
        servo2.detach();

        prev2 = d;

        pinMode(6, HIGH);
        delay(1000);

        servo2.attach(4);
        servo2.writeMicroseconds(40);
        delay(prev2 + 350);
        servo2.detach();

        prev2 = 0;
      }*/
    }
  }
}