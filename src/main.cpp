#include <HardwareSerial.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <FastLED.h>
#include <WebServer.h>
#include <ArduinoJson.h>
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
#define PANELS_NUMBER 4

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
const char *htmlPage = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Led Strip Controller</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            background-color: #f0f0f0;
        }

        .container {
            background: #fff;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }
        .buttons {
            display: flex;
            justify-content: space-between;
            gap: 10px; /* Khoảng cách giữa các nút */
        }

        .buttons button {
            padding: 10px 20px;
            font-size: 16px;
            cursor: pointer;
            border: none;
            border-radius: 4px;
            background-color: #007bff;
            color: #fff;
            transition: background-color 0.3s;
        }

        .buttons button:hover {
            background-color: #0056b3;
        }
        .input-group input {
            padding: 10px;
            font-size: 16px;
            border: 1px solid #ccc;
            border-radius: 4px;
            width: 100%;
            box-sizing: border-box;
        }
        .checkbox-group {
            margin-bottom: 20px;
            display: flex;
            flex-direction: row;
        }

        .checkbox-group label {
            display: block;
            margin-bottom: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="checkbox-group">
            <label><input type="checkbox" id="auto" onchange="updateCheckbox('auto')"> Auto</label>
        </div>
        <div class="input-group">
            <label for="brightness">Brightness:</label>
            <input type="number" id="brightness" placeholder="Brightness">
        </div>
        <div class="input-group">
            <label for="speed">Speed:</label>
            <input type="number" id="speed" placeholder="Speed">
        </div>
        <div class="input-group">
            <label for="leds">LEDs:</label>
            <input type="number" id="leds" placeholder="LEDs">
        </div>
        <div class="buttons">
            <button onclick="sendData('start')">Start</button>
            <button onclick="sendData('stop')">Stop</button>
            <button onclick="sendData('next')">Next</button>
            <button onclick="sendData('previous')">Previous</button>
            <button onclick="sendInputData()">Apply Config</button>
            <button onclick="sendLeds()">Set LEDs</button>
        </div>
    </div>

    <script>
        function updateCheckbox(id) {
            var checkbox = document.getElementById(id);
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/state?mode=" + (checkbox.checked ? "1" : "0"), true);
            xhr.send();
        }
        function sendData(action) {
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/" + action, true);
            xhr.send();
        }
        function sendInputData() {
          var brightness = document.getElementById('brightness').value;
          var speed = document.getElementById('speed').value;
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/inputs?brightness=" + brightness + "&speed=" + speed+ "&leds=" + leds, true);
          xhr.send();
        }
        function sendLeds() {
          var Leds = document.getElementById('leds').value;
          var xhr = new XMLHttpRequest();
          xhr.open("GET", "/setLeds?leds=" + Leds, true);
          xhr.send();
        }
    </script>
</body>
</html>
)=====";

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

  Serial.printf("x moi: %d", x_moi);
  Serial.printf("Y moi: %d", y_moi);
  Serial.println();
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
  // SerialBT.begin("ESP32Test");
  //setup_routing();
}

void Display_Point(int x, int y, int r, int g, int b)
{
  for (uint16_t i = 0; i < PANE_WIDTH; i++)
  {
    for (uint16_t j = 0; j < PANE_HEIGHT; j++)
    {

      if (i >= x - 1 && i <= x + 1 && j >= y - 1 && j <= y + 1)
      {
        if (j <= 32)
        {
          dma_display->drawPixelRGB888(i, j, r, g, b);
        }
        else
        {
          dma_display->drawPixelRGB888(i, j, r, b, g);
        }
      }
      else
        dma_display->drawPixelRGB888(i, j, 0, 0, 0);
    }
  }
}
void printData()
{
  Serial.println("Printing data array...");

  for (int i = 0; i < DATA_SIZE; i++)
  {
    // In giá trị RGB565 dưới dạng hex
    if (i % 16 == 0)
    { // In dòng mới sau mỗi 16 phần tử để dễ đọc
      Serial.println();
    }
    Serial.print("0x");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println("Data printing completed.");
}
bool mode = false;
void loop()
{
  //server.handleClient();
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
        int x = int(data[row][0]);
        int y = int(data[row][1]);
        int x_in = int(data[0][0]);
        int y_in = int(data[0][1]);

        if(x_in==0&&y_in==0){
          mode = true;
        }
        else if (x_in==255&&y_in==255)
        {
           mode =false;
        }
        if(mode){
          int *_range;
         _range = drawStickMan(x, y);
        }
        else{
          // measure x dimension
        int min_x = x - 1 - ((num_moi * 3) / 3);
        int max_x = x + 1 + ((num_moi * 3) / 3);
        int min_y = y - 1;
        int max_y = y + 1;
        for (int i = min_x; i <= max_x; i++)
          {
            for (int j = y - 1; j <= y + 1; j++)
            {

              if (j <= 32)
                dma_display->drawPixelRGB888(i, j, 214, 24, 192);
              else
                dma_display->drawPixelRGB888(i, j, 214, 192, 24);
              // reach moi
              if (gen_moi == false)
              {
                drawRandomPoint(x, y, 5 + (num_moi * 3), false);
                gen_moi = true;
              }
              if (min_x <= x_moi + 1 && max_x > x_moi - 1 && min_y <= y_moi + 1 && max_y >= y_moi - 1)
              {
                reached = true;
                num_moi += 1;
                gen_moi = false;
              }
            }
          }
          if (reached)
          {
            min_x = x - 1 - ((num_moi * 3) / 3);
            max_x = x + 1 + ((num_moi * 3) / 3);
            for (int i = min_x; i <= max_x; i++)
            {
              for (int j = y - 1; j <= y + 1; j++)
              {
                if (j <= 32)
                  dma_display->drawPixelRGB888(i, j, 214, 24, 192);
                else
                  dma_display->drawPixelRGB888(i, j, 214, 192, 24);
              }
            }
            reached = false;
          }

          if (gen_moi)
          {
            drawRandomPoint(x, y, 5 + (num_moi * 3), gen_moi);
          }
        }
        
      }
      // Reset chỉ số chỉ mục sau khi xử lý
      bufferIndex = 0;
      // Serial.printf("%d - %d",int(data[0][0]),int(data[0][1]));
      // Display_Point(int(data[0][0]),int(data[0][1]),214,24,192);
      // Display_Point(data,numRows,numCols,214,24,192);
    }
  }
#pragma endregion

  // readDataFromSerial();
}
