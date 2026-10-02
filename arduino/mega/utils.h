const int cell = 8;

byte arrowDown[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b10001,
  0b01010,
  0b00100,
  0b00000
};

byte arrowUp[8] = {
  0b00000,
  0b00100,
  0b01010,
  0b10001,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

byte checkMark[8] = {
  0b00000,
  0b00000,
  0b00001,
  0b00010,
  0b10100, 
  0b01000,
  0b00000,
  0b00000
};

String checkEdges(int M[cell][cell]) {
  for (int r = 0; r < 2; r++) {
    for (int c = 0; c < cell; c++) {
      if (M[r][c] != 1) {
        char col = 'a' + c;
        int row = r + 1;
        return String(col) + row;
      }
    }
  }

  for (int r = cell - 2; r < cell; r++) {
    for (int c = 0; c < cell; c++) {
      if (M[r][c] != 1) {
        char col = 'a' + c;
        int row = r + 1;
        return String(col) + row;
      }
    }
  }

  return "";
}

int splitString(String input, char output[10][32], char sep = ',') {
  if (input.length() == 0) {
    memset(output, 0, 10 * 32);
    return 0;
  }

  int start = 0;
  int size = 0;
  int len = input.length();

  while (start < len && size < 10) {
    int idx = input.indexOf(sep, start);
    
    if (idx == -1) {
      idx = len;
    }

    String temp = input.substring(start, idx);
    temp.trim();

    bool isDuplicate = false;
    for (int i = 0; i < size; i++) {
      if (temp.equals(output[i])) {
        isDuplicate = true;
        break;
      }
    }

    if (!isDuplicate && temp.length() > 0) {
      temp.toCharArray(output[size], 32);
      size++;
    }

    start = idx + 1;
  }

  return size;
}

int treatValue(int data) { return (data * 9 / 1024); }

const int MAX_KEYS = 20;
unsigned long intervals[MAX_KEYS];
unsigned long lastTimes[MAX_KEYS];

bool ddlay(unsigned long interval) {
  unsigned long now = millis();

  for (int i = 0; i < MAX_KEYS; i++) {
    if (intervals[i] == interval) {
      unsigned long elapsed = now - lastTimes[i];

      if (elapsed < 10) {
        return true;
      }

      if (elapsed >= interval) {
        lastTimes[i] = now;
        return true;
      }

      return false;
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