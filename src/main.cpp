#include <HardwareSerial.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <FastLED.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Fonts/FreeSans9pt7b.h>
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

  // Serial.printf("x moi: %d", x_moi);
  // Serial.printf("Y moi: %d", y_moi);
  // Serial.println();
}

int MinX_bar = 0;
int MaxX_bar = 5;
int MinY_bar = 50;
void Drawbar(int x)
{
  // measure x dimension
  int dodai = 5;

  MinX_bar = x - 1 - ((dodai * 3) / 3);
  MaxX_bar = x + 1 + ((dodai * 3) / 3);

  for (int i = MinX_bar; i <= MaxX_bar; i++)
  {
    for (int j = MinY_bar; j <= MinY_bar + 1; j++)
    {
      if (j <= 32)
        dma_display->drawPixelRGB888(i, j, 214, 24, 192);
      else
        dma_display->drawPixelRGB888(i, j, 214, 192, 24);
    }
  }
}

bool created = false;
int X_vat = 20;
int Y_vat = 2;
void DrawObject(bool deleted = false)
{
  // measure x dimension
  int dodai = 5;
  if (!created)
  {
    X_vat = random(20, 109); // random(min, max), max không bao gồm
    Y_vat = 2;
    created = true;
  }
  else
  {
    Y_vat += 1;
    if (Y_vat > 52)
      created = false;
  }
  int min_x = X_vat - 1;
  int max_x = X_vat + 1;
  int min_y = Y_vat - 1;
  int max_y = Y_vat + 1;
  if (deleted)
  {
    for (int i = min_x; i <= max_x; i++)
    {
      for (int j = min_y; j <= max_y; j++)
      {
        if (j <= 32)
          dma_display->drawPixelRGB888(i, j, 0, 0, 255);
        else
          dma_display->drawPixelRGB888(i, j, 0, 255, 0);
      }
    }
  }
  else
  {
    for (int i = min_x; i <= max_x; i++)
    {
      for (int j = min_y; j <= max_y; j++)
      {
        dma_display->drawPixelRGB888(i, j, 0, 0, 0);
      }
    }
  }
}

int score = 0;
#define COLOR_BLACK dma_display->color333(0, 0, 0)
#define COLOR_WHITE dma_display->color333(7, 7, 7)
void HeldObject()
{
  if (Y_vat >= MinY_bar && X_vat - 1 >= MinX_bar && X_vat + 1 <= MaxX_bar)
  {
    created = false;
    DrawObject(true);
    DrawObject();
    score += 1;
    dma_display->setFont(&FreeSans9pt7b); // Change to the desired font size
    dma_display->setTextColor(COLOR_WHITE);
    dma_display->setCursor(4, 4);
    dma_display->printf("Score: %d", score);
  }
}

// Stickman

int X_shot = 0;
int Y_shot = 32;
bool created_shot = false;
bool direction = false;
void Drawshot(bool deleted = false)
{
  // measure x dimension

  if (!created_shot)
  {
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
  }
  else
  {
    if (direction == false)
    {
      X_shot += 1;
    }
    else
    {
      X_shot -= 1;
    }
  }
  int min_x = X_shot - 1;
  int max_x = X_shot;
  int min_y = Y_shot - 1;
  int max_y = Y_shot;
  if (deleted)
  {
    for (int i = min_x; i <= max_x; i++)
    {
      for (int j = min_y; j <= max_y; j++)
      {
        if (j <= 32)
          dma_display->drawPixelRGB888(i, j, 0, 0, 255);
        else
          dma_display->drawPixelRGB888(i, j, 0, 255, 0);
      }
    }
  }
  else
  {
    for (int i = min_x; i <= max_x; i++)
    {
      for (int j = min_y; j <= max_y; j++)
      {
        dma_display->drawPixelRGB888(i, j, 0, 0, 0);
      }
    }
  }
}
int *drawStickMan(int x, int y)
{
  int range[4];
  // Vẽ đầu
  dma_display->drawCircle(x, y, 2, rgbToUint16(255, 0, 0, false)); // Đầu stick man

  // Vẽ thân
  dma_display->drawLine(x, y + 3, x, y + 7, rgbToUint16(255, 0, 0, false)); // Thân stick man

  // Vẽ tay
  dma_display->drawLine(x - 3, y + 4, x + 3, y + 4, rgbToUint16(255, 0, 0, false)); // Tay trái và tay phải
  range[0] = x - 3;
  range[1] = x + 3;
  // Vẽ chân
  dma_display->drawLine(x, y + 7, x - 3, y + 10, rgbToUint16(255, 0, 0, false)); // Chân trái
  dma_display->drawLine(x, y + 7, x + 3, y + 10, rgbToUint16(255, 0, 0, false)); // Chân phải

  range[2] = y - 2;
  range[3] = y + 10;

  return range;
}
void setup()
{
  Serial.begin(115200 * 2); // Đảm bảo baud rate khớp với baud rate của Python
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();            // setup the LED matrix
  dma_display->setBrightness8(50); // 0-255
  dma_display->clearScreen();
}

bool mode = false;
unsigned long period_time = millis();
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
  if (bufferIndex >= 2 * sizeof(int))
  {
    // Đã nhận đủ kích thước dữ liệu
    memcpy(&numRows, buffer, sizeof(int));
    memcpy(&numCols, buffer + sizeof(int), sizeof(int));

    int expectedSize = numRows * numCols * floatSize;

    if (bufferIndex >= (2 * sizeof(int) + expectedSize))
    {
      dma_display->clearScreen();
      float data[numRows][numCols];
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

      Drawbar(int(data[0][0]));
      unsigned long now = millis();
      if (now - period_time >= 600)
      {
        DrawObject();
        HeldObject();
        period_time = now;
      }

      // Reset chỉ số chỉ mục sau khi xử lý
      bufferIndex = 0;
    }
  }
#pragma endregion

  // readDataFromSerial();
}

// int x = int(data[row][0]);
// int y = int(data[row][1]);
// int x_in = int(data[0][0]);
// int y_in = int(data[0][1]);

// if (x_in == 0 && y_in == 0)
// {
//   mode = true;
// }
// else if (x_in == 255 && y_in == 255)
// {
//   mode = false;
// }
// if (mode)
// {
//   int *_range;
//   _range = drawStickMan(x, y);
// }
// else
// {
//   // measure x dimension
//   int min_x = x - 1 - ((num_moi * 3) / 3);
//   int max_x = x + 1 + ((num_moi * 3) / 3);
//   int min_y = y - 1;
//   int max_y = y + 1;
//   for (int i = min_x; i <= max_x; i++)
//   {
//     for (int j = y - 1; j <= y + 1; j++)
//     {
//       if (j <= 32)
//         dma_display->drawPixelRGB888(i, j, 214, 24, 192);
//       else
//         dma_display->drawPixelRGB888(i, j, 214, 192, 24);
//       // reach moi
//       if (gen_moi == false && y<255)
//       {
//         drawRandomPoint(x, y, 5 + (num_moi * 3), false);

//         gen_moi = true;
//       }
//       if (min_x <= x_moi + 1 && max_x > x_moi - 1 && min_y <= y_moi + 1 && max_y >= y_moi - 1)
//       {
//         reached = true;
//         num_moi += 1;
//         gen_moi = false;
//       }
//     }
//   }
//   if (reached)
//   {
//     min_x = x - 1 - ((num_moi * 3) / 3);
//     max_x = x + 1 + ((num_moi * 3) / 3);
//     for (int i = min_x; i <= max_x; i++)
//     {
//       for (int j = y - 1; j <= y + 1; j++)
//       {
//         if (j <= 32)
//           dma_display->drawPixelRGB888(i, j, 214, 24, 192);
//         else
//           dma_display->drawPixelRGB888(i, j, 214, 192, 24);
//       }
//     }
//     reached = false;
//   }
//   if (gen_moi)
//   {
//     drawRandomPoint(x, y, 5 + (num_moi * 3), gen_moi);
//   }
// }