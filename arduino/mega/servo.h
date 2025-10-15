#include <Servo.h>

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

int pinBreak = 48, pinBreakOutput = 46, pinMagnet = 6;
int prev = 0, prev1 = 90, prev2 = 0;
float dwn = 0;

struct Square {
  int turn;
  int move;
  int down;
};

extern Square S0 = {};
extern Square S[8][8] = {};

void serbegin() {
  Serial.begin(9600);

  pinMode(pinMagnet, LOW);
  pinMode(pinBreakOutput, OUTPUT);
  digitalWrite(pinBreakOutput, HIGH); 
  pinMode(pinBreak, INPUT_PULLUP);

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
      delay(20);
    }
  } else {
    for (int angolo = prev; angolo >= degree; angolo--) {
      servo3.write(angolo);
      servo4.write(180 - angolo);
      delay(20);
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
  for (int i = 1; i < ms / 100 + 1; i++) {
    if (digitalRead(pinBreak)) {
      dwn = 0;
      break;
    }

    servo2.attach(4);
    servo2.writeMicroseconds(1396);
    delay(100);
    servo2.detach();
    delay(10);
  }

  dwn = (dwn - ms < 0 ? 0 : dwn - ms) / 100;
}

void down(int ms) {
  for (int i = 1; i < ms / 100 + 1; i++) {
    servo2.attach(4);
    servo2.writeMicroseconds(1548);
    delay(100);
    servo2.detach();
    delay(10);
  }

  dwn += ms / 100;
}

void reset() {
  if (dwn > 0)
    up(dwn * 100);
  moveH(90);
  moveV(0);
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

    reset();
    moveH(h < 0 ? 0 : h);
    moveV(v < 0 ? 0 : v);
    down(d < 0 ? 0 : d * 100);
    delay(100);
    pinMode(pinMagnet, magnet);
    delay(100);
    reset();
  }
}

void eat() {
  reset();
  moveH(S0.turn < 0 ? 0 : S0.turn);
  moveV(S0.move < 0 ? 0 : S0.move);
  down(S0.down < 0 ? 0 : S0.down * 100);
  delay(100);
  pinMode(pinMagnet, 0);
  delay(100);
  reset();
}

void serloop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    Serial.println(input);

    if (input == "on") pinMode(pinMagnet, HIGH);
    else if (input == "off") pinMode(pinMagnet, LOW);
    else if (input == "reset") reset();
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
    } else {
      move(input, 1);
      if (input.length() > 2)
        move(input.substring(2), 0);
    }
  }
}