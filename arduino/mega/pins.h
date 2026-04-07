int rows[] = {51, 47, 43, 39, 35, 31, 27, 23};
int cols[] = {53, 49, 45, 41, 37, 33, 29, 25};

//                RS, E, D4, D5, D6, D7
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
int joyPin1 = A0;
int joyPin2 = A1;
int switchPin = 13;

int resetlcd = 53;

int pinBreak = 48, pinBreakOutput = 46, pinMagnet = 6;
int servoH = 5, servoM1 = 3, servoM2 = 2, servoUD = 4;