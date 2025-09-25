// Minesweeper - Image-driven (Adjusted: robust BG, button-number spacing, guanji toggle)
// Assumes: M5Unified library and image headers in sketch folder.
// Headers expected: bg.h, map1.h, map2.h, lei1.h, lei2.h, qi.h, button1.h, button2.h, jifen.h, shu.h, ying.h, guanji.h
// Each header should provide: <NAME>_IMG (uint16_t array) and <NAME>_IMG_WIDTH / <NAME>_IMG_HEIGHT macros.

#include <M5Unified.h>

// conditional includes (unchanged)
#if __has_include("bg.h")
  #include "bg.h"
  #define HAS_BG 1
#else
  #define HAS_BG 0
#endif

#if __has_include("map1.h")
  #include "map1.h"
  #define HAS_MAP1 1
#else
  #define HAS_MAP1 0
#endif

#if __has_include("map2.h")
  #include "map2.h"
  #define HAS_MAP2 1
#else
  #define HAS_MAP2 0
#endif

#if __has_include("lei1.h")
  #include "lei1.h"
  #define HAS_LEI1 1
#else
  #define HAS_LEI1 0
#endif

#if __has_include("lei2.h")
  #include "lei2.h"
  #define HAS_LEI2 1
#else
  #define HAS_LEI2 0
#endif

#if __has_include("qi.h")
  #include "qi.h"
  #define HAS_QI 1
#else
  #define HAS_QI 0
#endif

#if __has_include("button1.h")
  #include "button1.h"
  #define HAS_BUTTON1 1
#else
  #define HAS_BUTTON1 0
#endif

#if __has_include("button2.h")
  #include "button2.h"
  #define HAS_BUTTON2 1
#else
  #define HAS_BUTTON2 0
#endif

#if __has_include("jifen.h")
  #include "jifen.h"
  #define HAS_JIFEN 1
#else
  #define HAS_JIFEN 0
#endif

#if __has_include("shu.h")
  #include "shu.h"
  #define HAS_SHU 1
#else
  #define HAS_SHU 0
#endif

#if __has_include("ying.h")
  #include "ying.h"
  #define HAS_YING 1
#else
  #define HAS_YING 0
#endif

#if __has_include("guanji.h")
  #include "guanji.h"
  #define HAS_GUANJI 1
#else
  #define HAS_GUANJI 0
#endif

// --- layout constants (as before) ---
const int COLS = 9;
const int ROWS = 12;
const int CELL = 54;
const int GAP  = 3;
const int GRID_LEFT = 15;
const int GRID_TOP  = 150;
const int LEI_W = 27;
const int LEI_H = 27;
const int QI_W  = 27;
const int QI_H  = 27;
const int LEI_OFFSET_X = 12;
const int LEI_OFFSET_Y = 12;
const int BUTTON_W = 120;
const int BUTTON_H = 57;
const int BUTTON_GAP = 24;
const int BUTTON_LEFT_MARGIN = 9;
const int BUTTON_RIGHT_MARGIN = 9;
const int BUTTON_ROW_TOP_OFFSET = 33;
const unsigned long LONGPRESS_MS = 600;

// derived
int SCREEN_W = 0, SCREEN_H = 0;
int GRID_X = GRID_LEFT;
int GRID_Y = GRID_TOP;
int GRID_W = 0;
int GRID_H = 0;
int BUTTON_ROW_Y = 0;
int btn1_x=0, btn2_x=0, btn_restart_x=0;

// game state
uint8_t minesArr[ROWS][COLS];
uint8_t numsArr[ROWS][COLS];
uint8_t stateArr[ROWS][COLS]; // 0 hidden,1 revealed,2 flagged
bool gameOver = false;
int totalMines = 0;
int flagsLeft = 0;
long scorePoints = 0;

// touch
bool touching = false;
unsigned long touchStartMs = 0;
int lastTouchX = 0, lastTouchY = 0;
bool longHandled = false;

// guanji (lock wallpaper) state
bool showingGuanji = false;

// helper: pushImage with different header symbol names
void pushBgIfPresent() {
  // prefer BG_IMG_* macro style
  #if defined(BG_IMG_WIDTH) && defined(BG_IMG_HEIGHT)
    M5.Display.pushImage(0, 0, BG_IMG_WIDTH, BG_IMG_HEIGHT, (const uint16_t*)BG_IMG);
  #elif defined(BG_WIDTH) && defined(BG_HEIGHT)
    M5.Display.pushImage(0, 0, BG_WIDTH, BG_HEIGHT, (const uint16_t*)BG);
  #else
    // fallback fill
    M5.Display.fillScreen(TFT_WHITE);
  #endif
  // always commit immediately for background
  M5.Display.display();
}

// draw background (robust)
void drawBackground() {
  pushBgIfPresent();
}

// draw a single cell (unchanged except flagged uses map1)
void drawCell(int r, int c) {
  if (r<0||c<0||r>=ROWS||c>=COLS) return;
  int stride = CELL + GAP;
  int x = GRID_X + c * stride;
  int y = GRID_Y + r * stride;

  // background: flagged or hidden -> map1; revealed -> map2
  if (stateArr[r][c] == 1) {
    // revealed
    #if defined(MAP2_IMG_WIDTH) && defined(MAP2_IMG_HEIGHT)
      M5.Display.pushImage(x,y, MAP2_IMG_WIDTH, MAP2_IMG_HEIGHT, (const uint16_t*)MAP2_IMG);
    #else
      M5.Display.fillRect(x,y,CELL,CELL,TFT_WHITE);
      M5.Display.drawRect(x,y,CELL,CELL,TFT_BLACK);
    #endif
  } else {
    // hidden or flagged
    #if defined(MAP1_IMG_WIDTH) && defined(MAP1_IMG_HEIGHT)
      M5.Display.pushImage(x,y, MAP1_IMG_WIDTH, MAP1_IMG_HEIGHT, (const uint16_t*)MAP1_IMG);
    #else
      M5.Display.fillRect(x,y,CELL,CELL,0xC618);
      M5.Display.drawRect(x,y,CELL,CELL,TFT_BLACK);
    #endif
  }

  // flagged -> draw qi
  if (stateArr[r][c] == 2) {
    #if defined(QI_IMG_WIDTH) && defined(QI_IMG_HEIGHT)
      M5.Display.pushImage(x+LEI_OFFSET_X, y+LEI_OFFSET_Y, QI_IMG_WIDTH, QI_IMG_HEIGHT, (const uint16_t*)QI_IMG);
    #else
      M5.Display.fillRect(x+LEI_OFFSET_X,y+LEI_OFFSET_Y,QI_W,QI_H,TFT_RED);
    #endif
    return;
  }

  // revealed & mine -> use lei1
  if (stateArr[r][c] == 1 && minesArr[r][c]) {
    #if defined(LEI1_IMG_WIDTH) && defined(LEI1_IMG_HEIGHT)
      M5.Display.pushImage(x+LEI_OFFSET_X, y+LEI_OFFSET_Y, LEI1_IMG_WIDTH, LEI1_IMG_HEIGHT, (const uint16_t*)LEI1_IMG);
    #else
      M5.Display.fillRect(x+LEI_OFFSET_X,y+LEI_OFFSET_Y,LEI_W,LEI_H,TFT_BLACK);
    #endif
    return;
  }

  // revealed & number
  if (stateArr[r][c] == 1 && !minesArr[r][c]) {
    uint8_t n = numsArr[r][c];
    if (n>0) {
      String s = String(n);
      int tsize = 3;
      M5.Display.setTextSize(tsize);
      M5.Display.setTextColor(TFT_BLACK);
      // approximate char width for tsize=3
      int charW = 12;
      int textW = charW * s.length();
      int drawX = x + (CELL - textW)/2;
      int drawY = y + (CELL - 16)/2;
      if (drawX < x) drawX = x+2;
      M5.Display.drawString(s, drawX, drawY);
      M5.Display.setTextSize(1);
    }
  }
}

// draw full grid
void drawGridFull() {
  for (int r=0;r<ROWS;++r) for (int c=0;c<COLS;++c) drawCell(r,c);
}

// draw buttons and UI (with strict right-margin=15 for numbers)
void drawButtonsAndUI() {
  BUTTON_ROW_Y = GRID_Y + GRID_H + BUTTON_ROW_TOP_OFFSET;
  btn1_x = BUTTON_LEFT_MARGIN;
  int btn1_y = BUTTON_ROW_Y;
  btn2_x = btn1_x + BUTTON_W + BUTTON_GAP;
  int btn2_y = btn1_y;
  btn_restart_x = SCREEN_W - BUTTON_RIGHT_MARGIN - BUTTON_W;
  int btn_restart_y = btn1_y;

  // button backgrounds
  #if defined(BUTTON1_IMG_WIDTH) && defined(BUTTON1_IMG_HEIGHT)
    M5.Display.pushImage(btn1_x, btn1_y, BUTTON1_IMG_WIDTH, BUTTON1_IMG_HEIGHT, (const uint16_t*)BUTTON1_IMG);
    M5.Display.pushImage(btn2_x, btn2_y, BUTTON1_IMG_WIDTH, BUTTON1_IMG_HEIGHT, (const uint16_t*)BUTTON1_IMG);
  #else
    M5.Display.fillRect(btn1_x,btn1_y,BUTTON_W,BUTTON_H,0xFFDF);
    M5.Display.drawRect(btn1_x,btn1_y,BUTTON_W,BUTTON_H,TFT_BLACK);
    M5.Display.fillRect(btn2_x,btn2_y,BUTTON_W,BUTTON_H,0xFFDF);
    M5.Display.drawRect(btn2_x,btn2_y,BUTTON_W,BUTTON_H,TFT_BLACK);
  #endif

  #if defined(BUTTON2_IMG_WIDTH) && defined(BUTTON2_IMG_HEIGHT)
    M5.Display.pushImage(btn_restart_x, btn_restart_y, BUTTON2_IMG_WIDTH, BUTTON2_IMG_HEIGHT, (const uint16_t*)BUTTON2_IMG);
  #else
    M5.Display.fillRect(btn_restart_x,btn_restart_y,BUTTON_W,BUTTON_H,TFT_WHITE);
    M5.Display.drawRect(btn_restart_x,btn_restart_y,BUTTON_W,BUTTON_H,TFT_BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.drawString("Restart", btn_restart_x + 18, btn_restart_y + 12);
    M5.Display.setTextSize(1);
  #endif

  // icons
  int icon_x = btn1_x + 15;
  int icon_y = btn1_y + 12;
  #if defined(JIFEN_IMG_WIDTH) && defined(JIFEN_IMG_HEIGHT)
    M5.Display.pushImage(icon_x, icon_y, JIFEN_IMG_WIDTH, JIFEN_IMG_HEIGHT, (const uint16_t*)JIFEN_IMG);
  #else
    M5.Display.fillRect(icon_x,icon_y,27,27,TFT_YELLOW);
  #endif

  int mine_icon_x = btn2_x + 15;
  int mine_icon_y = btn2_y + 12;
  #if defined(LEI2_IMG_WIDTH) && defined(LEI2_IMG_HEIGHT)
    M5.Display.pushImage(mine_icon_x, mine_icon_y, LEI2_IMG_WIDTH, LEI2_IMG_HEIGHT, (const uint16_t*)LEI2_IMG);
  #else
    M5.Display.fillRect(mine_icon_x,mine_icon_y,27,27,TFT_BLACK);
  #endif

  // score number (right aligned 15px from right)
  {
    String s = String(scorePoints);
    int tsize = 3;
    M5.Display.setTextSize(tsize);
    M5.Display.setTextColor(TFT_BLACK);
    int charW = 12;
    int textW = charW * s.length();
    int drawRight = btn1_x + BUTTON_W - 15;
    int drawX = drawRight - textW;
    int iconCenterY = icon_y + 27/2;
    int drawY = iconCenterY - 8;
    if (drawX < btn1_x + 6) drawX = btn1_x + 6;
    M5.Display.drawString(s, drawX, drawY);
    M5.Display.setTextSize(1);
  }

  // mines left number (right aligned 15px)
  {
    String s = String(flagsLeft);
    int tsize = 3;
    M5.Display.setTextSize(tsize);
    M5.Display.setTextColor(TFT_BLACK);
    int charW = 12;
    int textW = charW * s.length();
    int drawRight = btn2_x + BUTTON_W - 15;
    int drawX = drawRight - textW;
    int iconCenterY = mine_icon_y + 27/2;
    int drawY = iconCenterY - 8;
    if (drawX < btn2_x + 6) drawX = btn2_x + 6;
    M5.Display.drawString(s, drawX, drawY);
    M5.Display.setTextSize(1);
  }
}

// reveal all mines and show shu image
void revealAllMines() {
  for (int r=0;r<ROWS;++r) for (int c=0;c<COLS;++c) if (minesArr[r][c]) { stateArr[r][c]=1; drawCell(r,c); }
  if (HAS_SHU) {
    #if defined(SHU_IMG_WIDTH) && defined(SHU_IMG_HEIGHT)
      M5.Display.pushImage((SCREEN_W - SHU_IMG_WIDTH)/2, (SCREEN_H - SHU_IMG_HEIGHT)/2, SHU_IMG_WIDTH, SHU_IMG_HEIGHT, (const uint16_t*)SHU_IMG);
    #endif
  } else {
    M5.Display.setTextSize(3);
    M5.Display.setTextColor(TFT_RED);
    M5.Display.drawString("GAME OVER", (SCREEN_W/2)-120, SCREEN_H/2 - 10);
    M5.Display.setTextSize(1);
  }
  M5.Display.display();
}

// flood reveal (calls display on each change for local refresh)
void revealFlood(int r,int c) {
  if (r<0||c<0||r>=ROWS||c>=COLS) return;
  if (stateArr[r][c] != 0) return;
  stateArr[r][c] = 1;
  scorePoints += 1;
  drawCell(r,c);
  M5.Display.display();
  if (numsArr[r][c] == 0) {
    for (int dr=-1; dr<=1; ++dr) for (int dc=-1; dc<=1; ++dc) {
      int nr=r+dr, nc=c+dc;
      if (nr==r && nc==c) continue;
      if (nr>=0 && nr<ROWS && nc>=0 && nc<COLS) revealFlood(nr,nc);
    }
  }
}

// check win
bool checkWin() {
  for (int r=0;r<ROWS;++r) for (int c=0;c<COLS;++c) if (!minesArr[r][c] && stateArr[r][c] != 1) return false;
  return true;
}

// reset board (full redraw)
void resetBoard() {
  int totalCells = ROWS*COLS;
  totalMines = max(10, int(totalCells * 0.16));
  flagsLeft = totalMines;
  scorePoints = 0;
  gameOver = false;
  for (int r=0;r<ROWS;++r) for (int c=0;c<COLS;++c) { minesArr[r][c]=0; numsArr[r][c]=0; stateArr[r][c]=0; }
  static int cells[ROWS*COLS];
  int idx=0;
  for (int r=0;r<ROWS;++r) for (int c=0;c<COLS;++c) cells[idx++]=r*COLS + c;
  for (int i=idx-1;i>0;--i) { int j=random(0,i+1); int t=cells[i]; cells[i]=cells[j]; cells[j]=t; }
  for (int i=0;i<totalMines && i<idx;++i) { int v=cells[i]; int rr=v/COLS, cc=v%COLS; minesArr[rr][cc]=1; }
  for (int r=0;r<ROWS;++r) for (int c=0;c<COLS;++c) {
    if (minesArr[r][c]) { numsArr[r][c] = 255; continue; }
    int cnt=0;
    for (int dr=-1;dr<=1;++dr) for (int dc=-1;dc<=1;++dc) {
      int nr=r+dr, nc=c+dc;
      if (nr>=0&&nr<ROWS&&nc>=0&&nc<COLS&&minesArr[nr][nc]) cnt++;
    }
    numsArr[r][c]=cnt;
  }

  // draw background + grid + ui (commit full screen once)
  drawBackground();
  GRID_W = COLS*CELL + (COLS-1)*GAP;
  GRID_H = ROWS*CELL + (ROWS-1)*GAP;
  drawGridFull();
  drawButtonsAndUI();
  M5.Display.display();
}

// handle short tap
void handleShortTap(int sx,int sy) {
  if (showingGuanji) return; // ignore while guanji shown
  // check within grid
  if (sx >= GRID_X && sx < GRID_X + GRID_W && sy >= GRID_Y && sy < GRID_Y + GRID_H) {
    int relx = sx - GRID_X, rely = sy - GRID_Y;
    int stride = CELL + GAP;
    int cc = relx / stride;
    int rr = rely / stride;
    int within_x = relx % stride, within_y = rely % stride;
    if (within_x >= CELL || within_y >= CELL) return; // gap
    int r = rr, c = cc;
    if (gameOver) return;
    if (stateArr[r][c] != 0) return;
    if (minesArr[r][c]) {
      revealAllMines();
      gameOver = true;
      return;
    } else {
      revealFlood(r,c);
      if (checkWin()) {
        scorePoints += 50;
        gameOver = true;
        if (HAS_YING) {
          #if defined(YING_IMG_WIDTH) && defined(YING_IMG_HEIGHT)
            M5.Display.pushImage((SCREEN_W - YING_IMG_WIDTH)/2, (SCREEN_H - YING_IMG_HEIGHT)/2, YING_IMG_WIDTH, YING_IMG_HEIGHT, (const uint16_t*)YING_IMG);
          #endif
          M5.Display.display();
        } else {
          M5.Display.setTextSize(3);
          M5.Display.setTextColor(0x07E0);
          M5.Display.drawString("YOU WIN!", (SCREEN_W/2)-120, SCREEN_H/2 - 10);
          M5.Display.setTextSize(1);
          M5.Display.display();
        }
      }
      drawButtonsAndUI();
      M5.Display.display();
      return;
    }
  }
  // restart button
  int bx = btn_restart_x, by = BUTTON_ROW_Y;
  if (sx >= bx && sx <= bx + BUTTON_W && sy >= by && sy <= by + BUTTON_H) { resetBoard(); return; }
}

// handle long press (flag)
void handleLongPress(int sx,int sy) {
  if (showingGuanji) return; // ignore while guanji shown
  if (sx >= GRID_X && sx < GRID_X + GRID_W && sy >= GRID_Y && sy < GRID_Y + GRID_H) {
    int relx = sx - GRID_X, rely = sy - GRID_Y;
    int stride = CELL + GAP;
    int cc = relx / stride;
    int rr = rely / stride;
    int within_x = relx % stride, within_y = rely % stride;
    if (within_x >= CELL || within_y >= CELL) return; // gap
    int r = rr, c = cc;
    if (gameOver) return;
    if (stateArr[r][c] == 0 && flagsLeft > 0) {
      stateArr[r][c] = 2;
      flagsLeft--;
      if (minesArr[r][c]) scorePoints += 10;
    } else if (stateArr[r][c] == 2) {
      if (minesArr[r][c]) scorePoints -= 10;
      stateArr[r][c] = 0;
      flagsLeft++;
    }
    drawCell(r,c);
    drawButtonsAndUI();
    M5.Display.display();
    return;
  }
  int bx = btn_restart_x, by = BUTTON_ROW_Y;
  if (sx >= bx && sx <= bx + BUTTON_W && sy >= by && sy <= by + BUTTON_H) { resetBoard(); return; }
}

// toggle guanji wallpaper via side button (BtnB)
void toggleGuanji() {
  showingGuanji = !showingGuanji;
  if (showingGuanji) {
    // show guanji wallpaper
    #if defined(GUANJI_IMG_WIDTH) && defined(GUANJI_IMG_HEIGHT)
      M5.Display.pushImage(0,0,GUANJI_IMG_WIDTH,GUANJI_IMG_HEIGHT,(const uint16_t*)GUANJI_IMG);
    #else
      // fallback to bg if available
      pushBgIfPresent();
    #endif
    M5.Display.display();
  } else {
    // restore current screen (redraw same game state)
    drawBackground();
    drawGridFull();
    drawButtonsAndUI();
    M5.Display.display();
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(0);
  SCREEN_W = M5.Display.width();
  SCREEN_H = M5.Display.height();
  GRID_W = COLS*CELL + (COLS-1)*GAP;
  GRID_H = ROWS*CELL + (ROWS-1)*GAP;
  randomSeed(esp_random() ^ millis());
  drawBackground();
  resetBoard();
}

void loop() {
  M5.update();

  // handle guanji button (physical side key)
  if (M5.BtnB.wasPressed()) {
    toggleGuanji();
    // short debounce
    delay(150);
    return;
  }

  // if showing guanji, ignore touch & other inputs (only BtnB restores)
  if (showingGuanji) {
    // still update M5 so BtnB events are processed
    delay(10);
    return;
  }

  int cnt = M5.Touch.getCount();
  if (cnt > 0) {
    auto d = M5.Touch.getDetail(0);
    lastTouchX = d.x; lastTouchY = d.y;
    if (!touching) {
      touching = true;
      touchStartMs = millis();
      longHandled = false;
    } else {
      unsigned long dur = millis() - touchStartMs;
      if (!longHandled && dur >= LONGPRESS_MS) {
        handleLongPress(lastTouchX, lastTouchY);
        longHandled = true;
      }
    }
  } else {
    if (touching) {
      touching = false;
      unsigned long dur = millis() - touchStartMs;
      if (!longHandled) handleShortTap(lastTouchX, lastTouchY);
    }
  }

  if (M5.BtnA.wasPressed()) resetBoard();
  delay(10);
}
