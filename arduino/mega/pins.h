int rows[] = {51, 47, 43, 39, 35, 31, 27, 23};
int cols[] = {53, 49, 45, 41, 37, 33, 29, 25};

//                RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);
int joyPin1 = A0;
int joyPin2 = A1;
int switchPin = 6;

int resetlcd = 53;

int pinBreak = 48, pinBreakOutput = 46, pinMagnet = 44;
int servoH = 5, servoM1 = 2, servoM2 = 3, servoUD = 4;