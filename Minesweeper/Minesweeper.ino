// Minesweeper — 快速标题显示（使用 pushImage，一次性显示整图）
// 依赖：M5Unified + title image header "title_img_alpha_white.h"
// header 必须包含： const uint16_t TITLE_IMG[]; #define TITLE_IMG_WIDTH 540; #define TITLE_IMG_HEIGHT 150

#include <M5Unified.h>
#include "title_img_alpha_white.h"

// configuration
const int CELL = 60;
const int MAX_COLS = 9;
const int MAX_ROWS = 12;
const unsigned long LONGPRESS_MS = 600; // ms

// screen / grid
int SCREEN_W = 960;
int SCREEN_H = 540;
int colsActive = MAX_COLS;
int rowsActive = 9;
int GRID_W = 0;
int GRID_H = 0;
int GRID_X = 0;
int GRID_Y = 0; // will be set based on title height

int minesCount = 10;
int flagsLeft = 0;

// board arrays
uint8_t minesArr[MAX_ROWS][MAX_COLS];
uint8_t numsArr[MAX_ROWS][MAX_COLS];
uint8_t stateArr[MAX_ROWS][MAX_COLS]; // 0 hidden,1 revealed,2 flagged

bool gameOver = false;

// touch tracking
bool touching = false;
unsigned long touchStartMs = 0;
int lastRawX = 0, lastRawY = 0;
bool longHandled = false;

// UI rects
struct Rect { int x, y, w, h; };
Rect restartBtnRect;
Rect minesBoxRect;

// prototypes
void drawTitleOnce();
void drawUI();
void drawGridFull();
void drawCell(int r, int c);
void resetBoard();
void revealFlood(int r, int c);
void revealAllMines();
bool checkWin();
bool pixelToCell(int sx, int sy, int &outR, int &outC);

// --- setup / loop ---
void setup() {
  Serial.begin(115200);
  delay(10);

  auto cfg = M5.config();
  M5.begin(cfg);

  // rotation and sizes
  M5.Display.setRotation(0);
  SCREEN_W = M5.Display.width();
  SCREEN_H = M5.Display.height();
  Serial.printf("Display W=%d H=%d\n", SCREEN_W, SCREEN_H);

  colsActive = MAX_COLS;
  GRID_X = (SCREEN_W - colsActive * CELL) / 2;
  GRID_W = colsActive * CELL;

  // ensure title fits — if too wide, we center with tx=0 fallback
  int title_w = TITLE_IMG_WIDTH;
  if (title_w > SCREEN_W) title_w = SCREEN_W;

  GRID_Y = TITLE_IMG_HEIGHT + 6; // put grid under title (6px margin)

  // rows under title, leave bottom UI ~60px
  rowsActive = (SCREEN_H - GRID_Y - 60) / CELL;
  if (rowsActive > MAX_ROWS) rowsActive = MAX_ROWS;
  if (rowsActive < 3) rowsActive = 3;
  GRID_H = rowsActive * CELL;

  minesCount = max(10, int(colsActive * rowsActive * 0.16));
  flagsLeft = minesCount;

  // UI rects
  int ui_h = 42;
  minesBoxRect.x = 10;
  minesBoxRect.y = SCREEN_H - ui_h - 8;
  minesBoxRect.w = 140;
  minesBoxRect.h = ui_h;

  restartBtnRect.w = 140;
  restartBtnRect.h = ui_h;
  restartBtnRect.x = SCREEN_W - restartBtnRect.w - 10;
  restartBtnRect.y = SCREEN_H - ui_h - 8;

  // Clear once, draw title immediately (fast)
  M5.Display.clear(TFT_WHITE);

  // Draw title once immediately with block transfer
  drawTitleOnce();

  // init game
  randomSeed(esp_random() ^ millis());
  resetBoard();

  // draw UI (bottom boxes etc) and draw grid content (cells)
  drawUI();
  drawGridFull();

  Serial.printf("Grid X=%d Y=%d W=%d H=%d rows=%d cols=%d mines=%d\n",
                GRID_X, GRID_Y, GRID_W, GRID_H, rowsActive, colsActive, minesCount);
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
    } else {
      unsigned long held = millis() - touchStartMs;
      if (!longHandled && held >= LONGPRESS_MS) {
        // long press handling: place/remove flag OR long-press restart
        int r, c;
        if (pixelToCell(lastRawX, lastRawY, r, c)) {
          if (!gameOver) {
            if (stateArr[r][c] == 0 && flagsLeft > 0) {
              stateArr[r][c] = 2; flagsLeft--;
            } else if (stateArr[r][c] == 2) {
              stateArr[r][c] = 0; flagsLeft++;
            }
            drawCell(r, c);
            // only update UI parts that changed (flags count)
            // redraw mines-left box value
            M5.Display.fillRect(minesBoxRect.x + 8, minesBoxRect.y + 4, minesBoxRect.w - 16, minesBoxRect.h - 8, TFT_BLACK);
            M5.Display.setTextSize(3);
            M5.Display.setTextColor(TFT_WHITE);
            M5.Display.drawString(String(flagsLeft), minesBoxRect.x + 12, minesBoxRect.y + 6);
            M5.Display.setTextSize(1);
          }
        } else {
          // maybe long press restart
          if (lastRawX >= restartBtnRect.x && lastRawX <= restartBtnRect.x + restartBtnRect.w &&
              lastRawY >= restartBtnRect.y && lastRawY <= restartBtnRect.y + restartBtnRect.h) {
            // full redraw only on restart (user requested)
            resetBoard();         // resetBoard will redraw grid/UI
          }
        }
        longHandled = true;
      }
    }
  } else {
    if (touching) {
      // touch released
      touching = false;
      if (!longHandled) {
        int r, c;
        if (pixelToCell(lastRawX, lastRawY, r, c)) {
          if (!gameOver && stateArr[r][c] == 0) {
            if (minesArr[r][c]) {
              revealAllMines();
              gameOver = true;
              // show GAME OVER (no full clear)
              M5.Display.setTextSize(2);
              M5.Display.setTextColor(TFT_RED);
              M5.Display.drawString("GAME OVER", SCREEN_W/2 - 60, SCREEN_H/2 - 10);
              M5.Display.setTextSize(1);
            } else {
              revealFlood(r, c);
              if (checkWin()) {
                M5.Display.setTextSize(2);
                M5.Display.setTextColor(TFT_GREEN);
                M5.Display.drawString("YOU WIN!", SCREEN_W/2 - 60, SCREEN_H/2 - 10);
                M5.Display.setTextSize(1);
                gameOver = true;
              }
            }
          }
        } else {
          // tap outside grid -> may be restart button
          if (lastRawX >= restartBtnRect.x && lastRawX <= restartBtnRect.x + restartBtnRect.w &&
              lastRawY >= restartBtnRect.y && lastRawY <= restartBtnRect.y + restartBtnRect.h) {
            resetBoard(); // full redraw only on restart
          }
        }
      }
    }
  }

  if (M5.BtnA.wasPressed()) {
    resetBoard();
  }

  delay(8);
}

// ---------- image + UI drawing ----------
// Use pushImage to draw TITLE_IMG in a single fast operation.
// NOTE: pushImage signature varies by library; for M5Unified (LovyanGFX), this is commonly supported:
//   M5.Display.pushImage(x, y, w, h, (const uint16_t*)buf);
// If your build errors saying pushImage is not found, paste the compiler error here and I'll adapt.
void drawTitleOnce() {
  int tx = (SCREEN_W - TITLE_IMG_WIDTH) / 2;
  if (tx < 0) tx = 0;
  // fast block transfer
  M5.Display.pushImage(tx, 0, TITLE_IMG_WIDTH, TITLE_IMG_HEIGHT, (const uint16_t*)TITLE_IMG);
}

// draw UI components (does not clear title)
void drawUI() {
  // bottom UI area
  M5.Display.fillRect(0, SCREEN_H - 80, SCREEN_W, 80, TFT_WHITE);

  // mines-left box (left-bottom) - black box with white number
  M5.Display.fillRect(minesBoxRect.x, minesBoxRect.y, minesBoxRect.w, minesBoxRect.h, TFT_BLACK);
  M5.Display.drawRect(minesBoxRect.x, minesBoxRect.y, minesBoxRect.w, minesBoxRect.h, TFT_WHITE);
  M5.Display.setTextSize(3);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.drawString(String(flagsLeft), minesBoxRect.x + 12, minesBoxRect.y + 6);
  M5.Display.setTextSize(1);

  // restart button (right-bottom)
  M5.Display.fillRect(restartBtnRect.x, restartBtnRect.y, restartBtnRect.w, restartBtnRect.h, TFT_WHITE);
  M5.Display.drawRect(restartBtnRect.x, restartBtnRect.y, restartBtnRect.w, restartBtnRect.h, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_BLACK);
  M5.Display.drawString("Restart", restartBtnRect.x + 18, restartBtnRect.y + 10);
  M5.Display.setTextSize(1);

  // grid border
  M5.Display.drawRect(GRID_X - 2, GRID_Y - 2, GRID_W + 4, GRID_H + 4, TFT_BLACK);
}

// draw all cells (initial or full redraw)
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
    // hidden
    M5.Display.fillRect(x, y, CELL - 2, CELL - 2, 0xC0C0C0);
    M5.Display.drawRect(x, y, CELL - 2, CELL - 2, TFT_BLACK);
  } else if (stateArr[r][c] == 2) {
    // flagged: white 'F' on gray
    M5.Display.fillRect(x, y, CELL - 2, CELL - 2, 0xC0C0C0);
    M5.Display.drawRect(x, y, CELL - 2, CELL - 2, TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.drawString("F", x + CELL / 3, y + CELL / 4);
    M5.Display.setTextSize(1);
  } else {
    // revealed
    M5.Display.fillRect(x, y, CELL - 2, CELL - 2, TFT_WHITE);
    M5.Display.drawRect(x, y, CELL - 2, CELL - 2, TFT_BLACK);
    if (minesArr[r][c]) {
      M5.Display.setTextSize(2);
      M5.Display.setTextColor(TFT_RED);
      M5.Display.drawString("*", x + CELL / 3, y + CELL / 4);
      M5.Display.setTextSize(1);
    } else {
      uint8_t n = numsArr[r][c];
      if (n > 0) {
        uint16_t col = TFT_BLUE;
        if (n == 2) col = 0x008000;
        if (n == 3) col = TFT_RED;
        M5.Display.setTextSize(2);
        M5.Display.setTextColor(col);
        M5.Display.drawString(String(n), x + CELL / 3, y + CELL / 4);
        M5.Display.setTextSize(1);
      }
    }
  }
}

// reset board: full redraw only here (user requested)
void resetBoard() {
  // full clear and redraw title/UI/grid
  M5.Display.clear(TFT_WHITE);
  drawTitleOnce();

  flagsLeft = minesCount;
  gameOver = false;

  // reset arrays
  for (int r = 0; r < MAX_ROWS; ++r) {
    for (int c = 0; c < MAX_COLS; ++c) {
      minesArr[r][c] = 0;
      numsArr[r][c] = 0;
      stateArr[r][c] = 0;
    }
  }

  // build cell list
  int total = rowsActive * colsActive;
  static int cells[MAX_ROWS * MAX_COLS][2];
  int idx = 0;
  for (int r = 0; r < rowsActive; ++r) {
    for (int c = 0; c < colsActive; ++c) {
      cells[idx][0] = r;
      cells[idx][1] = c;
      idx++;
    }
  }

  // Fisher-Yates shuffle
  for (int i = total - 1; i > 0; --i) {
    int j = random(0, i + 1);
    int tx = cells[i][0], ty = cells[i][1];
    cells[i][0] = cells[j][0]; cells[i][1] = cells[j][1];
    cells[j][0] = tx; cells[j][1] = ty;
  }

  // place mines
  for (int i = 0; i < minesCount && i < total; ++i) {
    int rr = cells[i][0], cc = cells[i][1];
    minesArr[rr][cc] = 1;
  }

  // compute neighbor counts
  for (int r = 0; r < rowsActive; ++r) {
    for (int c = 0; c < colsActive; ++c) {
      if (minesArr[r][c]) { numsArr[r][c] = 255; continue; }
      int cnt = 0;
      for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
          int nr = r + dr, nc = c + dc;
          if (nr >= 0 && nr < rowsActive && nc >= 0 && nc < colsActive && minesArr[nr][nc]) cnt++;
        }
      }
      numsArr[r][c] = cnt;
    }
  }

  // full UI redraw
  drawUI();
  drawGridFull();
}

void revealFlood(int r, int c) {
  if (r < 0 || c < 0 || r >= rowsActive || c >= colsActive) return;
  if (stateArr[r][c] != 0) return;
  stateArr[r][c] = 1;
  drawCell(r, c);
  if (numsArr[r][c] == 0) {
    for (int dr = -1; dr <= 1; ++dr) for (int dc = -1; dc <= 1; ++dc)
      if (!(dr == 0 && dc == 0)) revealFlood(r + dr, c + dc);
  }
}

void revealAllMines() {
  for (int r = 0; r < rowsActive; ++r) for (int c = 0; c < colsActive; ++c)
    if (minesArr[r][c]) { stateArr[r][c] = 1; drawCell(r, c); }
}

bool checkWin() {
  for (int r = 0; r < rowsActive; ++r) for (int c = 0; c < colsActive; ++c)
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
