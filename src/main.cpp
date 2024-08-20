#include <HardwareSerial.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <Fonts/FreeMono9pt7b.h>
#include <gif_frames.h>
#include <spacecraft.h>
#pragma region Matrix Config
#define R1_PIN 4
#define G1_PIN 5
#define B1_PIN 6
#define R2_PIN 7
#define G2_PIN 15
#define B2_PIN 16
#define A_PIN 18
#define B_PIN 8 // Changed from library default
#define C_PIN 3
#define D_PIN 42
#define E_PIN 39 // required for 1/32 scan panels, like 64x64px. Any available pin would do, i.e. IO32
#define LAT_PIN 40
#define OE_PIN 2
#define CLK_PIN 41

// Configure for your panel(s) as appropriate!
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64
#define PANELS_NUMBER 4

#define PANE_WIDTH PANEL_WIDTH *PANELS_NUMBER
#define PANE_HEIGHT PANEL_HEIGHT
#define NUM_PIXELS PANE_WIDTH *PANE_HEIGHT

#define COLOR_GREEN 0x07E0
#define COLOR_RED 0xF800
#define COLOR_BLUE 0x001F
#define COLOR_YELLOW 0xFFE0
#define COLOR_WHITE 0xFFFF
#define COLOR_BLACK 0x0000

MatrixPanel_I2S_DMA *dma_display = nullptr;
// Another way of creating config structure
// Custom pin mapping for all pins
HUB75_I2S_CFG::i2s_pins _pins = {R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};
HUB75_I2S_CFG mxconfig(
    PANEL_WIDTH,           // width
    PANEL_HEIGHT,          // height
    PANELS_NUMBER,         // chain length
    _pins,                 // pin mapping
    HUB75_I2S_CFG::FM6126A // driver chip
);
#pragma endregion
const int bufferSize = 8192; // Kích thước bộ đệm tùy chỉnh
uint8_t buffer[bufferSize];
int bufferIndex = 0;
int numRows = 0;
int numCols = 0;
const int floatSize = sizeof(float);
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 128;
const int PLAYER_WIDTH = frame_width;
const int PLAYER_HEIGHT = frame_height;
const int ENEMY_WIDTH = 8;
const int ENEMY_HEIGHT = 8;
const int BULLET_WIDTH = 2;
const int BULLET_HEIGHT = 4;
const int NUM_ENEMIES = 10;
const int MAX_BULLETS = 10; // Số lượng viên đạn tối đa
struct Player
{
  int x, y;
  bool alive;
};

struct Enemy
{
  int x, y;
  bool alive;
};

struct Bullet
{
  int x, y;
  bool active;
};

uint32_t rgb565_to_rgb888(uint16_t rgb565)
{
  // Tách các kênh màu từ giá trị RGB565
  uint8_t r = (rgb565 >> 11) & 0x1F; // 5 bit cho đỏ
  uint8_t g = (rgb565 >> 5) & 0x3F;  // 6 bit cho xanh lục
  uint8_t b = rgb565 & 0x1F;         // 5 bit cho xanh dương

  // Chuyển đổi các kênh màu từ 5/6 bit sang 8 bit
  r = (r * 255 + 15) / 31; // Chuyển đổi từ 5 bit sang 8 bit
  g = (g * 255 + 31) / 63; // Chuyển đổi từ 6 bit sang 8 bit
  b = (b * 255 + 15) / 31; // Chuyển đổi từ 5 bit sang 8 bit

  // Ghép các giá trị RGB888 thành một giá trị 32 bit
  return (r << 16) | (g << 8) | b;
}
Player player;
std::vector<Enemy> enemies(NUM_ENEMIES);
Bullet bullets[MAX_BULLETS];
void Initialize_State()
{
  player = {SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - 10, true};
  for (int i = 0; i < NUM_ENEMIES; ++i)
  {
    enemies[i] = {i * (SCREEN_WIDTH / NUM_ENEMIES)+7, 0, true};
  }
  for (int i = 0; i < MAX_BULLETS; ++i)
  {
    bullets[i] = {0, 0, false};
  }
}
void fireBullet()
{
  for (int i = 0; i < MAX_BULLETS; ++i)
  {
    if (!bullets[i].active)
    {
      bullets[i].x = player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2;
      bullets[i].y = player.y - BULLET_HEIGHT;
      bullets[i].active = true;
      break; // Chỉ bắn một viên đạn tại một thời điểm
    }
  }
}
int X_player = 0;
void updatePlayer(int x)
{
  // Example: Move player left or right based on input
  player.x = x;
}

unsigned long last_enemies_update = millis();
int enemies_update_rate = 500;
void updateEnemies()
{
  unsigned long now = millis();
  if (now - last_enemies_update < enemies_update_rate)
    return;
  last_enemies_update = now;
  for (auto &enemy : enemies)
  {
    if (enemy.alive)
    {
      enemy.y += 1; // Move downwards
      if (enemy.y > SCREEN_HEIGHT)
      {
        enemy.y = 0; // Reset position if off screen
      }
    }
  }
}

unsigned long last_bullets_update = millis();
int bullets_update_speed = 10;
void updateBullets()
{
  unsigned long now = millis();
  if (now - last_bullets_update < bullets_update_speed)
    return;
  last_bullets_update = now;
  for (int i = 0; i < MAX_BULLETS; ++i)
  {
    if (bullets[i].active)
    {
      bullets[i].y -= 4; // Di chuyển lên trên
      if (bullets[i].y < 0)
      {
        bullets[i].active = false; // Vô hiệu hóa nếu ra ngoài màn hình
      }
    }
  }
}
unsigned long last_game_over_time = millis();
int restart_time = 2000;
void checkCollisions()
{
  // Check collision between bullet and enemies
  for (int i = 0; i < MAX_BULLETS; ++i)
  {
    if (bullets[i].active)
    {
      for (auto &enemy : enemies)
      {
        if (enemy.alive &&
            bullets[i].x < enemy.x + ENEMY_WIDTH &&
            bullets[i].x + BULLET_WIDTH > enemy.x &&
            bullets[i].y < enemy.y + ENEMY_HEIGHT &&
            bullets[i].y + BULLET_HEIGHT > enemy.y)
        {
          enemy.alive = false;
          bullets[i].active = false;
          break;
        }
      }
    }
  }

  // Check collision between enemies and player
  for (auto &enemy : enemies)
  {
    if (enemy.alive &&
        player.x < enemy.x + ENEMY_WIDTH &&
        player.x + PLAYER_WIDTH > enemy.x &&
        player.y < enemy.y + ENEMY_HEIGHT &&
        player.y + PLAYER_HEIGHT > enemy.y)
    {
      player.alive = false; // Player is hit
      last_game_over_time = millis();
    }
  }
}
unsigned long last_render_time = millis();
int render_rate = 20; // Frame rate in frames per second

void S_frame_0(int x, int y)
{
  for (int i = 0; i < frame_width; i++)
  {
    for (int j = 0; j < frame_height; j++)
    {
      int x1 = x + i;
      int y1 = y + j;
      if (y1 > 64)
      {
        y1 = y1 - 64;
        x1 = x1 + 128;
      }
      uint16_t color = gif_frame_0[i + j * frame_width];
      dma_display->drawPixel(x1, y1, color);
    }
  }
}

void S_frame_1(int x, int y)
{
  for (int i = 0; i < frame_width; i++)
  {
    for (int j = 0; j < frame_height; j++)
    {
      int x1 = x + i;
      int y1 = y + j;
      if (y1 > 64)
      {
        y1 = y1 - 64;
        x1 = x1 + 128;
      }
      uint16_t color = gif_frame_1[i + j * frame_width];
      dma_display->drawPixel(x1, y1, color);
    }
  }
}

void S_frame_2(int x, int y)
{
  for (int i = 0; i < frame_width; i++)
  {
    for (int j = 0; j < frame_height; j++)
    {
      int x1 = x + i;
      int y1 = y + j;
      if (y1 > 64)
      {
        y1 = y1 - 64;
        x1 = x1 + 128;
      }
      uint16_t color = gif_frame_2[i + j * frame_width];
      dma_display->drawPixel(x1, y1, color);
    }
  }
}
void S_frame_3(int x, int y)
{
  for (int i = 0; i < frame_width; i++)
  {
    for (int j = 0; j < frame_height; j++)
    {
      int x1 = x + i;
      int y1 = y + j;
      if (y1 > 64)
      {
        y1 = y1 - 64;
        x1 = x1 + 128;
      }
      uint16_t color = gif_frame_3[i + j * frame_width];
      dma_display->drawPixel(x1, y1, color);
    }
  }
}
void S_frame_4(int x, int y)
{
  for (int i = 0; i < frame_width; i++)
  {
    for (int j = 0; j < frame_height; j++)
    {
      int x1 = x + i;
      int y1 = y + j;
      if (y1 > 64)
      {
        y1 = y1 - 64;
        x1 = x1 + 128;
      }
      uint16_t color = gif_frame_4[i + j * frame_width];
      dma_display->drawPixel(x1, y1, color);
    }
  }
}
void S_frame_5(int x, int y)
{
  for (int i = 0; i < frame_width; i++)
  {
    for (int j = 0; j < frame_height; j++)
    {
      int x1 = x + i;
      int y1 = y + j;
      if (y1 > 64)
      {
        y1 = y1 - 64;
        x1 = x1 + 128;
      }
      uint16_t color = gif_frame_5[i + j * frame_width];
      dma_display->drawPixel(x1, y1, color);
    }
  }
}
void S_frame_6(int x, int y)
{
  for (int i = 0; i < frame_width; i++)
  {
    for (int j = 0; j < frame_height; j++)
    {
      int x1 = x + i;
      int y1 = y + j;
      if (y1 > 64)
      {
        y1 = y1 - 64;
        x1 = x1 + 128;
      }
      uint16_t color = gif_frame_6[i + j * frame_width];
      dma_display->drawPixel(x1, y1, color);
    }
  }
}
void S_frame_7(int x, int y)
{
  for (int i = 0; i < frame_width; i++)
  {
    for (int j = 0; j < frame_height; j++)
    {
      int x1 = x + i;
      int y1 = y + j;
      if (y1 > 64)
      {
        y1 = y1 - 64;
        x1 = x1 + 128;
      }
      uint16_t color = gif_frame_7[i + j * frame_width];
      dma_display->drawPixel(x1, y1, color);
    }
  }
}
unsigned long last_frame_time = millis();
int frame_rate =4; // Frame rate in frames per second
int frame_count = -1;
void draw_spacecraft_2(int x, int y)
{
  unsigned long now = millis();
  if(now - last_frame_time >= 1000/frame_rate){
    frame_count++;
    last_frame_time = now;
  }
  switch (frame_count)
  {
  case 7:
    S_frame_7(x, y);
    break;
  case 6:
    S_frame_6(x, y);
    break;
  case 5:
    S_frame_5(x, y);
    break;
  case 4:
    S_frame_4(x, y);
    break;
  case 3:
    S_frame_3(x, y);
    break;
  case 2:
    S_frame_2(x, y);
    break;
  case 1:
    S_frame_1(x, y);
    break;
  case 0:
    S_frame_0(x, y);
    break;
  default:
    S_frame_0(x, y);
    break;
  }
  if (frame_count == num_frames-1)
    frame_count = -1;
}

void draw_frame(int x, int y)
{
  for (int i = 0; i < ENEMY_WIDTH; i++)
  {
    for (int j = 0; j < ENEMY_HEIGHT; j++)
    {
      int x1 = x + i;
      int y1 = y + j;
      if (y1 > 64)
      {
        y1 = y1 - 64;
        x1 = x1 + 128;
      }
      uint16_t color = aircraft[j * ENEMY_WIDTH + i];
      dma_display->drawPixel(x1, y1, color);
    }
  }
}
void render()
{
  unsigned long now = millis();
  if (now - last_render_time < 1000 / render_rate)
    return;
  last_render_time = now;
  dma_display->clearScreen();
  if (player.alive)
  {
    int x = player.x;
    int y = player.y;
    if (y > 64)
    {
      y = y - 64;
      x = x + 128;
    }
    draw_spacecraft_2(x,y);
    // dma_display->drawRect(x, y, PLAYER_WIDTH, PLAYER_HEIGHT, COLOR_GREEN);
  }
  for (auto &enemy : enemies)
  {
    if (enemy.alive)
    {
      int x = enemy.x;
      int y = enemy.y;
      
      draw_frame(x, y);
      //dma_display->drawRect(x, y, ENEMY_WIDTH, ENEMY_HEIGHT, COLOR_RED);
    }
  }
  for (int i = 0; i < MAX_BULLETS; ++i)
  {
    if (bullets[i].active)
    {
      int x = bullets[i].x;
      int y = bullets[i].y;
      if (y > 64)
      {
        y = y - 64;
        x = x + 128;
      }
      dma_display->drawRect(x, y, BULLET_WIDTH, BULLET_HEIGHT, COLOR_WHITE);
    }
  }
}

unsigned long period_time = millis();
bool set_FPS(int rate)
{
  unsigned long now = millis();
  int elapsed = 1000 / rate;
  if (now - period_time >= elapsed)
  {
    period_time = now;
    return true;
  }
  else
    return false;
}
const char *text = "Game Over";
bool game_over = false;

void Game_Over()
{
  if(!game_over){
    game_over = true;
    last_game_over_time = millis();
  }
  // Font metrics
  int textWidth = 0;
  int textHeight = 0;
  int cursorX = 0;
  int cursorY = 0;

  // Calculate text dimensions (approximated for example)
  textWidth = strlen(text) * 11; // Assuming each character is approximately 6 pixels wide
  textHeight = 8;               // Assuming font height is 8 pixels

  // Compute the center position
  cursorX = (128 - textWidth) / 2;
  cursorY = (64 + textHeight) / 2;

  // Set the cursor position
  dma_display->setCursor(cursorX, cursorY);

  // Print text on the matrix
  dma_display->print(text);
}
void setup()
{
  Serial.begin(115200 * 2); // Đảm bảo baud rate khớp với baud rate của Python
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();             // setup the LED matrix
  dma_display->setBrightness8(200); // 0-255
  dma_display->clearScreen();
  dma_display->setTextColor(dma_display->color333(7, 7, 7)); //)
  dma_display->setTextSize(1);
  dma_display->setFont(&FreeMono9pt7b);
  Initialize_State();
}


void loop()
{
#pragma region Serial processing
  while (Serial.available() > 0)
  {
    uint8_t byteReceived = Serial.read();
    if (bufferIndex < bufferSize)
    {
      buffer[bufferIndex++] = byteReceived;
    }
    else
    {
      // Xử lý tình huống quá tải
      Serial.println("Buffer overflow");
      bufferIndex = 0; // Reset buffer index in case of overflow
    }
  }

  float data[numRows][numCols];
  if (bufferIndex >= 2 * sizeof(int))
  {
    // Đã nhận đủ kích thước dữ liệu
    memcpy(&numRows, buffer, sizeof(int));
    memcpy(&numCols, buffer + sizeof(int), sizeof(int));

    int expectedSize = numRows * numCols * floatSize;
    if (bufferIndex >= (2 * sizeof(int) + expectedSize))
    {
      for (int row = 0; row < numRows; ++row)
      {
        for (int col = 0; col < numCols; ++col)
        {
          int index = (2 * sizeof(int)) + (row * numCols + col) * floatSize;
          float value;
          memcpy(&value, &buffer[index], floatSize);
          data[row][col] = value;
        }
      }
      if (set_FPS(20))
      {
        if (player.alive)
        {
          int x = (int)data[0][0];
          int f = (int)data[0][2];
          updatePlayer(x);
          if (f == 1)
            fireBullet();
        }
        else
        {
          //Game_Over();
        }
      }
      // Reset chỉ số chỉ mục sau khi xử lý
    }
    bufferIndex = 0;
  }
  if(!player.alive) Game_Over();
  unsigned long now = millis();
  if(now-last_game_over_time >= restart_time && !player.alive){
    Initialize_State();
  }
  //if(game_over) return;
  updateEnemies();
  updateBullets();
  checkCollisions();
  render();
#pragma endregion
}
