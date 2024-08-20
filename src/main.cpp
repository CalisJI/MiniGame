#include <HardwareSerial.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
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
const int PLAYER_WIDTH = 16;
const int PLAYER_HEIGHT = 8;
const int ENEMY_WIDTH = 16;
const int ENEMY_HEIGHT = 8;
const int BULLET_WIDTH = 2;
const int BULLET_HEIGHT = 4;
const int NUM_ENEMIES = 10;


struct Player {
    int x, y;
    bool alive;
};

struct Enemy {
    int x, y;
    bool alive;
};

struct Bullet {
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
Player player ;
std::vector<Enemy> enemies(NUM_ENEMIES);
Bullet bullet = {0, 0, false};
void Initialize_State()
{
  player = {SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - 10, true};
  for (int i = 0; i < NUM_ENEMIES; ++i)
  {
    enemies[i] = {i * (SCREEN_WIDTH / NUM_ENEMIES), 0, true};
  }
}

void updatePlayer(int x) {
    // Example: Move player left or right based on input
    player.x = x;
}
void updateEnemies() {
    for (auto& enemy : enemies) {
        if (enemy.alive) {
            enemy.y += 1; // Move downwards
            if (enemy.y > SCREEN_HEIGHT) {
                enemy.y = 0; // Reset position if off screen
            }
        }
    }
}
void updateBullet() {
    if (bullet.active) {
        bullet.y -= 4; // Move upwards
        if (bullet.y < 0) {
            bullet.active = false; // Deactivate if off screen
        }
    }
}

void checkCollisions() {
    // Check collision between bullet and enemies
    for (auto& enemy : enemies) {
        if (enemy.alive && bullet.active && 
            bullet.x < enemy.x + ENEMY_WIDTH &&
            bullet.x + BULLET_WIDTH > enemy.x &&
            bullet.y < enemy.y + ENEMY_HEIGHT &&
            bullet.y + BULLET_HEIGHT > enemy.y) {
            enemy.alive = false;
            bullet.active = false;
        }
    }

    // Check collision between enemies and player
    for (auto& enemy : enemies) {
        if (enemy.alive && 
            player.x < enemy.x + ENEMY_WIDTH &&
            player.x + PLAYER_WIDTH > enemy.x &&
            player.y < enemy.y + ENEMY_HEIGHT &&
            player.y + PLAYER_HEIGHT > enemy.y) {
            player.alive = false; // Player is hit
        }
    }
}
void render() {
    dma_display->clearScreen();
    if (player.alive) {
        dma_display->drawRect(player.x, player.y, PLAYER_WIDTH, PLAYER_HEIGHT, COLOR_GREEN);
    }

    for (const auto& enemy : enemies) {
        if (enemy.alive) {
            dma_display->drawRect(enemy.x, enemy.y, ENEMY_WIDTH, ENEMY_HEIGHT, COLOR_RED);
        }
    }

    if (bullet.active) {
        dma_display->drawRect(bullet.x, bullet.y, BULLET_WIDTH, BULLET_HEIGHT, COLOR_WHITE);
    }
}

unsigned long period_time = millis();
bool set_FPS(int rate){
  unsigned long now = millis();
  int elapsed = 1000/rate;
  if(now - period_time >=elapsed){
    period_time = now;
    return true;
  }
  else return false;
}
void setup()
{
  Serial.begin(115200 * 2); // Đảm bảo baud rate khớp với baud rate của Python
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();             // setup the LED matrix
  dma_display->setBrightness8(200); // 0-255
  dma_display->clearScreen();
}

void loop()
{
  // server.handleClient();

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
      //dma_display->clearScreen();

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
          updatePlayer(x);
          updateEnemies();
          updateBullet();
          checkCollisions();
          render();
        }
      }

      // Reset chỉ số chỉ mục sau khi xử lý
    }
    bufferIndex = 0;
  }
#pragma endregion

}
