#include <HardwareSerial.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <FastLED.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Fonts/FreeMono9pt7b.h>
#pragma region Matrix Config
#define R1_PIN 25
#define G1_PIN 26
#define B1_PIN 27
#define R2_PIN 14
#define G2_PIN 12
#define B2_PIN 13
#define A_PIN 23
#define B_PIN 22 // Changed from library default
#define C_PIN 5
#define D_PIN 17
#define E_PIN 32 // required for 1/32 scan panels, like 64x64px. Any available pin would do, i.e. IO32
#define LAT_PIN 4
#define OE_PIN 15
#define CLK_PIN 16
// Configure for your panel(s) as appropriate!
#define PANEL_WIDTH 64
#define PANEL_HEIGHT 64
#define PANELS_NUMBER 2

#define PANE_WIDTH PANEL_WIDTH *PANELS_NUMBER
#define PANE_HEIGHT PANEL_HEIGHT
#define NUM_PIXELS PANE_WIDTH *PANE_HEIGHT
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
const int DATA_SIZE = 8192; // Số lượng phần tử trong mảng data
uint16_t data[DATA_SIZE];   // Mảng để lưu trữ dữ liệu RGB565
const int floatSize = sizeof(float);
static int x_moi = 0;
static int y_moi = 0;
int num_moi = 0;
bool reached = false;
bool gen_moi = false;
unsigned long period_time = millis();

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
void drawXbm565(int x, int y, int width, int height, const char *xbm, uint16_t color = 0xffff)
{
  if (width % 8 != 0)
  {
    width = ((width / 8) + 1) * 8;
  }
  for (int i = 0; i < width * height / 8; i++)
  {
    unsigned char charColumn = pgm_read_byte(xbm + i);
    for (int j = 0; j < 8; j++)
    {
      int targetX = (i * 8 + j) % width + x;
      int targetY = (8 * i / (width)) + y;
      if (bitRead(charColumn, j))
      {
        dma_display->drawPixel(targetX, targetY, color);
      }
    }
  }
}

uint16_t rgbToUint16(uint8_t r, uint8_t g, uint8_t b, bool mode)
{
  if (mode)
  {
    uint16_t red = (r >> 3) & 0x1F;   // Chuyển từ 8 bit xuống 5 bit
    uint16_t green = (g >> 2) & 0x3F; // Chuyển từ 8 bit xuống 6 bit
    uint16_t blue = (b >> 3) & 0x1F;  // Chuyển từ 8 bit xuống 5 bit

    return (red << 11) | (green << 5) | blue;
  }
  else
  {
    uint16_t red = (r >> 3) & 0x1F;   // Chuyển từ 8 bit xuống 5 bit
    uint16_t green = (b >> 2) & 0x3F; // Chuyển từ 8 bit xuống 6 bit
    uint16_t blue = (g >> 3) & 0x1F;  // Chuyển từ 8 bit xuống 5 bit

    return (red << 11) | (green << 5) | blue;
  }
}
void draw_true_color(int x, int y, int r, int g, int b)
{
  if (y <= 31)
  {
    dma_display->drawPixelRGB888(x, y, r, g, b);
  }
  else
  {
    dma_display->drawPixelRGB888(x, y, r, b, g);
  }
}
// Hàm kiểm tra xem điểm (x, y) có đủ xa điểm (x_ref, y_ref) không
bool isFarEnough(int x, int x_ref, int min_distance)
{
  int dx = x - x_ref;
  return (dx * dx) >= (min_distance * min_distance);
}

// Hàm tạo điểm ngẫu nhiên không gần với điểm đã cho
void drawRandomPoint(int x_ref, int y_ref, int min_distance, bool generated)
{
  // int x, y;
  bool point_found = false;
  if (generated)
  {
    for (int i = x_moi - 1; i <= x_moi + 1; i++)
    {
      for (int j = y_moi - 1; j <= y_moi + 1; j++)
      {
        draw_true_color(i, j, 0, 0, 255);
      }
    }
  }
  else
  {
    while (!point_found)
    {
      x_moi = random(PANE_WIDTH);
      y_moi = y_ref;
      // Kiểm tra khoảng cách giữa điểm ngẫu nhiên và điểm đã cho
      if (isFarEnough(x_moi, x_ref, min_distance) && x_moi >= 32 && x_moi < 220)
      {
        point_found = true;
      }
    }
    for (int i = x_moi - 1; i < x_moi + 1; i++)
    {
      for (int j = y_moi - 1; j < y_moi + 1; j++)
      {
        draw_true_color(i, j, 0, 0, 255);
      }
    }
  }
}

int MinX_bar = 0;
int MaxX_bar = 5;
int MinY_bar = 50;
void Drawbar(int x)
{
  // measure x dimension
  int dodai = 16;
  MinX_bar = x - dodai / 2;
  MaxX_bar = x + dodai / 2;
  dma_display->drawLine(MinX_bar, MinY_bar - 1, MaxX_bar, MinY_bar - 1, dma_display->color333(3, 3, 3));
  dma_display->drawLine(MinX_bar, MinY_bar, MaxX_bar, MinY_bar, dma_display->color333(3, 3, 3));
}

bool created = false;
int X_vat = 20;
int Y_vat = 2;
int lastpoint[2];
int r = 2;
int speed = 150;
void DrawObject(bool deleted = false)
{
  // measure x dimension
  unsigned long now = millis();
  if (!created)
  {
    lastpoint[0] = X_vat;
    lastpoint[1] = Y_vat;
    dma_display->fillCircle(X_vat, Y_vat, r, dma_display->color333(0, 0, 0));
    dma_display->drawLine(MinX_bar, MinY_bar - 1, MaxX_bar, MinY_bar - 1, dma_display->color333(3, 3, 3));
    dma_display->drawLine(MinX_bar, MinY_bar, MaxX_bar, MinY_bar, dma_display->color333(3, 3, 3));

    X_vat = random(20, 109); // random(min, max), max không bao gồm
    Y_vat = 2;
    created = true;
  }
  else
  {
    if (now - period_time >= speed)
    {
      dma_display->fillCircle(X_vat, Y_vat, r, dma_display->color333(0, 0, 0));
      Y_vat += 1;
      if (Y_vat > 52)
        created = false;
      period_time = now;
    }
  }

  // dma_display->drawCircle(X_vat,Y_vat,1,dma_display->color333(7,0,0));
  dma_display->fillCircle(X_vat, Y_vat, r, dma_display->color333(7, 0, 0));
}

int score = 0;
#define COLOR_BLACK dma_display->color333(0, 0, 0)
#define COLOR_WHITE dma_display->color333(7, 7, 7)
void HeldObject()
{
  if (Y_vat + r >= MinY_bar - 1 && X_vat >= MinX_bar - r && X_vat <= MaxX_bar + r)
  {
    Serial.println("Created");
    created = false;
    // DrawObject(true);
    DrawObject();

    dma_display->setTextColor(COLOR_BLACK);
    dma_display->setCursor(2, 11);
    dma_display->printf("%d", score);
    score += 1;
    dma_display->setFont(&FreeMono9pt7b); // Change to the desired font size
    dma_display->setTextColor(COLOR_WHITE);

    dma_display->setCursor(2, 11);
    dma_display->printf("%d", score);
  }
  else
  {
    dma_display->setFont(&FreeMono9pt7b); // Change to the desired font size
    dma_display->setTextColor(COLOR_WHITE);
    dma_display->setCursor(2, 11);
    dma_display->printf("%d", score);
  }
}

// Stickman

int X_shot = 0;
int Y_shot = 32;
bool created_shot = false;

bool direction = false; // false = left, true = right;
int lastpoint_shot[2];
unsigned long period_shot = millis();
int speed_shot = 100;
void Drawbullet(bool deleted = false)
{
  // measure x dimension
  unsigned long now = millis();
  if (!created_shot)
  {
    lastpoint_shot[0] = X_shot;
    lastpoint_shot[1] = Y_shot;
    dma_display->fillCircle(X_shot, Y_shot, 1, dma_display->color333(0, 0, 0));
    int ran = random(1, 3);
    if (ran == 1)
    {
      X_shot = 1;
      direction = false;
    }
    else
    {
      X_shot = 126;
      direction = true;
    }
    created_shot = true;
    dma_display->clearScreen();
    dma_display->fillCircle(X_shot, Y_shot, 1, dma_display->color333(5, 0, 0));
  }
  else
  {
    if (now - period_shot >= speed_shot)
    {
      dma_display->fillCircle(lastpoint_shot[0], lastpoint_shot[1], 1, dma_display->color333(0, 0, 0));

      if (direction == false)
      {
        X_shot += 1;
        if (X_shot >= 126)
          created_shot = false;
      }
      else
      {
        X_shot -= 1;
        if (X_shot <= 2)
          created_shot = false;
      }
      lastpoint_shot[0] = X_shot;
      lastpoint_shot[1] = Y_shot;
      dma_display->fillCircle(X_shot, Y_shot, 1, dma_display->color333(5, 0, 0));
      period_shot = now;
    }
  }
  // dma_display->fillCircle(X_shot,Y_shot,1,dma_display->color333(5,0,0));

  // dma_display->drawCircle(X_vat,Y_vat,1,dma_display->color333(7,0,0));
}

int current_sheildX = 0;
int current_sheildY = 0;
int length_shield = 11;
int current_stickmanX = 0;
int current_stickmanY = 0;
int Shield_open = 0;
unsigned long period_gameover = millis();
void drawStickMan(int x, int y, int defend = 0)
{
  // Vẽ đầu
  dma_display->drawCircle(x, y, 2, rgbToUint16(255, 0, 0, false)); // Đầu stick man

  // Vẽ thân
  dma_display->drawLine(x, y + 3, x, y + 7, rgbToUint16(255, 0, 0, false)); // Thân stick man

  // Vẽ tay
  dma_display->drawLine(x - 3, y + 4, x + 3, y + 4, rgbToUint16(255, 0, 0, false)); // Tay trái và tay phải
  // Vẽ chân
  dma_display->drawLine(x, y + 7, x - 3, y + 10, rgbToUint16(255, 0, 0, false)); // Chân trái
  dma_display->drawLine(x, y + 7, x + 3, y + 10, rgbToUint16(255, 0, 0, false)); // Chân phải
  current_stickmanX = x;
  current_stickmanY = y;
  if (defend == 2)
  {
    dma_display->drawLine(x - 4, y - 1, x - 4, y + 10, rgbToUint16(255, 0, 0, false));
    dma_display->drawLine(x - 5, y - 1, x - 5, y + 10, rgbToUint16(255, 0, 0, false));
    current_sheildX = x - 5;
    current_sheildY = y - 1;
    length_shield = 11;
  }
  else if (defend == 1)
  {
    dma_display->drawLine(x + 4, y - 1, x + 4, y + 10, rgbToUint16(255, 0, 0, false));
    dma_display->drawLine(x + 5, y - 1, x + 5, y + 10, rgbToUint16(255, 0, 0, false));
    current_sheildX = x + 5;
    current_sheildY = y - 1;
    length_shield = 11;
  }
  else
  {
    current_sheildX = 0;
    current_sheildY = 0;
  }
}
bool game_over = false;
void Defend()
{
  if (!direction)
  {
    // Do duoc ben trai
    if (current_sheildX != 0)
    {
      // co khien trai
      if (current_sheildX == current_stickmanX - 5)
      {
        if (X_shot >= current_sheildX)
        {
          created_shot = false;
        }
        else
        {
          // do nothing
        }
      }
      // ko co khien trai
      else
      {
        if (X_shot >= current_stickmanX - 5)
        {
          game_over = true;
          dma_display->setFont(&FreeMono9pt7b); // Change to the desired font size
          dma_display->setTextColor(COLOR_WHITE);
          dma_display->setCursor(2 + 16, 11 + 20);
          dma_display->print("GAME OVER");
          period_gameover = millis();
        }
        else
        {
          // do nothing
        }
      }
    }
  }
  else
  {
    if (current_sheildX != 0)
    {
      // co khien phai
      if (current_sheildX == current_stickmanX + 5)
      {
        if (X_shot <= current_sheildX)
        {
          created_shot = false;
        }
        else
        {
          // do nothing
          created_shot = true;
        }
      }
      // ko co khien trai
      else
      {
        if (X_shot <= current_stickmanX + 5)
        {
          game_over = true;
          dma_display->setFont(&FreeMono9pt7b); // Change to the desired font size
          dma_display->setTextColor(COLOR_WHITE);
          dma_display->setCursor(2 + 16, 11 + 20);
          dma_display->print("GAME OVER");
          period_gameover = millis();
        }
        else
        {
          // do nothing
        }
      }
    }
  }
}
void setup()
{
  Serial.begin(115200 * 2); // Đảm bảo baud rate khớp với baud rate của Python
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();            // setup the LED matrix
  dma_display->setBrightness8(200); // 0-255
  dma_display->clearScreen();
  // draw_cat(10,10,20,20);
  // drawXbm565(10,10,20,20,CAT_bits,dma_display->color565(100,0,0));
}

bool mode = false;
int reset_stickman = 2000;
bool Mode_game = false;
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

    if (numRows == 1 && numCols == 0)
    {
      Mode_game = true;
      //dma_display->clearScreen();
      // Serial.println("Stick Man");
    }
    else if (numRows == 0 && numCols == 0)
    {
      Mode_game = false;
      // Serial.println("HungVat");
      //dma_display->clearScreen();
    }
    
    int expectedSize = numRows * numCols * floatSize;
    if (bufferIndex >= (2 * sizeof(int) + expectedSize))
    {
      dma_display->clearScreen();

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

      if (!Mode_game)
      {
        Drawbar(int(data[0][0]));
      }
      else
      {
        if (!game_over)
          drawStickMan(int(data[0][0]), int(data[0][1]), int(data[0][2]));
      }

      // Reset chỉ số chỉ mục sau khi xử lý
      
    }
    bufferIndex = 0;
  }
  if (Mode_game)
  {
    if (game_over)
    {
      if (millis() - period_gameover >= reset_stickman)
      {
        game_over = false;
        dma_display->clearScreen();

      }
    }
  }

  // Stickman region
  if (Mode_game)
  {
    Drawbullet();
    Defend();
  }
  else // Hung Vat
  {
    DrawObject(true);
    DrawObject();
    HeldObject();
  }
#pragma endregion
}
