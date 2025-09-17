/*
  Minesweeper (improved)
  - M5Unified, Paper S3
  - dynamic rows filling bottom blank space, white 'F', big mines-left number,
    full-screen redraw on reset, hold-based long-press.
*/

#include <M5Unified.h>

// display / grid params
const int CELL = 60;          // cell size (px)
const int TOPBAR = 56;        // top UI height
const int MAX_COLS = 9;       // fixed columns
const int MAX_ROWS = 12;      // upper bound for safety

// interaction
const unsigned long LONGPRESS_MS = 600;

// dynamic/calculated
int SCREEN_W = 960;
int SCREEN_H = 540;
int colsActive = MAX_COLS;
int rowsActive = 9;          // will compute in setup
int GRID_W = 0;
int GRID_H = 0;
int GRID_X = 0;
int GRID_Y = TOPBAR;

// mines config (computed)
int minesCount = 10;
int flagsLeft = 0;

// game arrays (max-sized)
uint8_t minesArr[MAX_ROWS][MAX_COLS];
uint8_t numsArr[MAX_ROWS][MAX_COLS];
uint8_t stateArr[MAX_ROWS][MAX_COLS]; // 0 hidden,1 revealed,2 flagged

bool gameOver = false;

// touch tracking
int prevCnt = 0;
bool touching = false;
unsigned long touchStartMs = 0;
int lastRawX = 0, lastRawY = 0;
bool longHandled = false;

void drawUI();
void drawGridFull();
void drawCell(int r, int c);
void resetBoard();
void revealFlood(int r, int c);
void revealAllMines();
bool checkWin();
bool pixelToCell(int sx, int sy, int &outR, int &outC);

void setup() {
  Serial.begin(115200);
  delay(10);

  auto cfg = M5.config();
  M5.begin(cfg);

  M5.Display.setRotation(0);
  SCREEN_W = M5.Display.width();
  SCREEN_H = M5.Display.height();

  // columns fixed, compute rows that fit in remaining area
  colsActive = MAX_COLS;
  rowsActive = (SCREEN_H - TOPBAR) / CELL;
  if (rowsActive > MAX_ROWS) rowsActive = MAX_ROWS;
  if (rowsActive < 3) rowsActive = 3; // safety minimum

  GRID_W = colsActive * CELL;
  GRID_H = rowsActive * CELL;
  GRID_X = (SCREEN_W - GRID_W) / 2;
  GRID_Y = TOPBAR;

  // compute mines count ~16% density, at least 10
  minesCount = max(10, int(colsActive * rowsActive * 0.16));
  flagsLeft = minesCount;

  // initial display setup & full clear
  M5.Display.clear(TFT_WHITE);
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_BLACK);

  randomSeed(esp_random() ^ millis());

  resetBoard();
  drawUI();
  drawGridFull();

  Serial.printf("Screen %dx%d, cols=%d rows=%d, mines=%d\n", SCREEN_W, SCREEN_H, colsActive, rowsActive, minesCount);
}

void loop() {
  M5.update();

  int cnt = M5.Touch.getCount();

  if (cnt > 0) {
    const auto &d = M5.Touch.getDetail(0);
    lastRawX = d.x;
    lastRawY = d.y;
    if (!touching) {
      touching = true;
      touchStartMs = millis();
      longHandled = false;
      Serial.printf("DOWN raw(%d,%d)\n", lastRawX, lastRawY);
    } else {
      unsigned long held = millis() - touchStartMs;
      if (!longHandled && held >= LONGPRESS_MS) {
        int r, c;
        if (pixelToCell(lastRawX, lastRawY, r, c)) {
          Serial.printf("LONG handled cell (%d,%d)\n", r, c);
          if (!gameOver) {
            if (stateArr[r][c] == 0 && flagsLeft > 0) {
              stateArr[r][c] = 2; flagsLeft--;
            } else if (stateArr[r][c] == 2) {
              stateArr[r][c] = 0; flagsLeft++;
            }
            drawCell(r, c);
            drawUI();
          }
        } else {
          Serial.println("LONG handled outside");
        }
        longHandled = true;
      }
    }
  } else {
    if (touching) {
      touching = false;
      unsigned long dur = millis() - touchStartMs;
      Serial.printf("UP raw(%d,%d) dur=%lu longHandled=%d\n", lastRawX, lastRawY, dur, longHandled ? 1:0);
      if (!longHandled) {
        int r, c;
        if (pixelToCell(lastRawX, lastRawY, r, c)) {
          Serial.printf("SHORT TAP cell (%d,%d)\n", r, c);
          if (!gameOver && stateArr[r][c] == 0) {
            if (minesArr[r][c]) {
              revealAllMines();
              gameOver = true;
              drawUI();
              M5.Display.setTextSize(2);
              M5.Display.setTextColor(TFT_RED);
              M5.Display.drawString("GAME OVER", SCREEN_W/2 - 60, SCREEN_H/2 - 10);
              M5.Display.setTextSize(1);
              Serial.println("GAME OVER");
            } else {
              revealFlood(r, c);
              drawUI();
              if (checkWin()) {
                M5.Display.setTextSize(2);
                M5.Display.setTextColor(TFT_GREEN);
                M5.Display.drawString("YOU WIN!", SCREEN_W/2 - 60, SCREEN_H/2 - 10);
                M5.Display.setTextSize(1);
                gameOver = true;
                Serial.println("YOU WIN");
              }
            }
          }
        } else {
          // top-right area restart
          if (lastRawX >= SCREEN_W - 160 && lastRawY <= TOPBAR + 20) {
            Serial.println("Top-right restart tapped");
            resetBoard();
            drawUI();
            drawGridFull();
          } else {
            Serial.println("SHORT TAP outside grid");
          }
        }
      } else {
        Serial.println("UP after longHandled");
      }
    }
  }

  if (M5.BtnA.wasPressed()) {
    Serial.println("Reset by BtnA");
    resetBoard();
    drawUI();
    drawGridFull();
  }

  delay(10);
}

// ---------- Drawing & logic ----------

void drawUI() {
  // full top clear: ensures previous game over text is removed
  M5.Display.fillRect(0, 0, SCREEN_W, TOPBAR, TFT_WHITE);

  // Title
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_BLACK);
  M5.Display.drawString("Minesweeper", 10, 8);

  // Big mines-left number
  M5.Display.setTextSize(3); // larger as requested
  M5.Display.setTextColor(TFT_RED);
  // Put count near left, slightly lower
  M5.Display.drawString(String("Left: ") + String(flagsLeft), 10, 30);

  // small restart hint top-right
  M5.Display.setTextSize(1);
  M5.Display.setTextColor(TFT_BLACK);
  M5.Display.drawString("Tap here to Restart", SCREEN_W - 160, 8);

  // grid border
  M5.Display.drawRect(GRID_X - 2, GRID_Y - 2, GRID_W + 4, GRID_H + 4, TFT_BLACK);
}

void drawGridFull() {
  for (int r = 0; r < rowsActive; ++r) {
    for (int c = 0; c < colsActive; ++c) {
      drawCell(r, c);
    }
  }
}

void drawCell(int r, int c) {
  if (r < 0 || r >= rowsActive || c < 0 || c >= colsActive) return;
  int x = GRID_X + c * CELL;
  int y = GRID_Y + r * CELL;

  if (stateArr[r][c] == 0) {
    // hidden: gray background
    M5.Display.fillRect(x, y, CELL - 2, CELL - 2, 0xC0C0C0);
    M5.Display.drawRect(x, y, CELL - 2, CELL - 2, TFT_BLACK);
  } else if (stateArr[r][c] == 2) {
    // flagged: white 'F' on gray bg (as you requested)
    M5.Display.fillRect(x, y, CELL - 2, CELL - 2, 0xC0C0C0);
    M5.Display.drawRect(x, y, CELL - 2, CELL - 2, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE); // white F
    M5.Display.drawString("F", x + CELL/3, y + CELL/4);
    M5.Display.setTextSize(1);
  } else {
    // revealed
    M5.Display.fillRect(x, y, CELL - 2, CELL - 2, TFT_WHITE);
    M5.Display.drawRect(x, y, CELL - 2, CELL - 2, TFT_BLACK);
    if (minesArr[r][c]) {
      M5.Display.setTextSize(2);
      M5.Display.setTextColor(TFT_RED);
      M5.Display.drawString("*", x + CELL/3, y + CELL/4);
      M5.Display.setTextSize(1);
    } else {
      uint8_t n = numsArr[r][c];
      if (n > 0) {
        uint16_t col = TFT_BLUE;
        if (n == 2) col = 0x008000;
        if (n == 3) col = TFT_RED;
        M5.Display.setTextSize(2);
        M5.Display.setTextColor(col);
        M5.Display.drawString(String(n), x + CELL/3, y + CELL/4);
        M5.Display.setTextSize(1);
      }
    }
  }
}

void resetBoard() {
  // full clear to remove any overlay text
  M5.Display.clear(TFT_WHITE);

  flagsLeft = minesCount;
  gameOver = false;

  // clear arrays
  for (int r = 0; r < MAX_ROWS; ++r)
    for (int c = 0; c < MAX_COLS; ++c) {
      minesArr[r][c] = 0;
      numsArr[r][c] = 0;
      stateArr[r][c] = 0;
    }

  // create list of active cells
  int total = rowsActive * colsActive;
  int cells[rowsActive * colsActive][2];
  int idx = 0;
  for (int r = 0; r < rowsActive; ++r)
    for (int c = 0; c < colsActive; ++c) {
      cells[idx][0] = r;
      cells[idx][1] = c;
      idx++;
    }

  // Fisher-Yates shuffle
  for (int i = total - 1; i > 0; --i) {
    int j = random(0, i + 1);
    int tx = cells[i][0], ty = cells[i][1];
    cells[i][0] = cells[j][0]; cells[i][1] = cells[j][1];
    cells[j][0] = tx; cells[j][1] = ty;
  }

  // place mines (minesCount may be dynamic)
  for (int i = 0; i < minesCount && i < total; ++i) {
    int rr = cells[i][0], cc = cells[i][1];
    minesArr[rr][cc] = 1;
  }

  // compute neighbor counts
  for (int r = 0; r < rowsActive; ++r) {
    for (int c = 0; c < colsActive; ++c) {
      if (minesArr[r][c]) {
        numsArr[r][c] = 255;
        continue;
      }
      int cnt = 0;
      for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
          int nr = r + dr, nc = c + dc;
          if (nr >= 0 && nr < rowsActive && nc >= 0 && nc < colsActive && minesArr[nr][nc]) cnt++;
        }
      numsArr[r][c] = cnt;
    }
  }

  // redraw UI & grid
  drawUI();
  drawGridFull();
}

void revealFlood(int r, int c) {
  if (r < 0 || c < 0 || r >= rowsActive || c >= colsActive) return;
  if (stateArr[r][c] != 0) return;
  stateArr[r][c] = 1;
  drawCell(r, c);
  if (numsArr[r][c] == 0) {
    for (int dr = -1; dr <= 1; ++dr)
      for (int dc = -1; dc <= 1; ++dc)
        if (!(dr == 0 && dc == 0)) revealFlood(r + dr, c + dc);
  }
}

void revealAllMines() {
  for (int r = 0; r < rowsActive; ++r)
    for (int c = 0; c < colsActive; ++c)
      if (minesArr[r][c]) {
        stateArr[r][c] = 1;
        drawCell(r, c);
      }
}

bool checkWin() {
  for (int r = 0; r < rowsActive; ++r)
    for (int c = 0; c < colsActive; ++c)
      if (!minesArr[r][c] && stateArr[r][c] != 1) return false;
  return true;
}

bool pixelToCell(int sx, int sy, int &outR, int &outC) {
  if (sy < GRID_Y) return false;
  if (sx < GRID_X || sx >= GRID_X + GRID_W) return false;
  int cx = (sx - GRID_X) / CELL;
  int cy = (sy - GRID_Y) / CELL;
  if (cx < 0 || cx >= colsActive || cy < 0 || cy >= rowsActive) return false;
  outR = cy; outC = cx;
  return true;
}
