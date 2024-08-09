#include <HardwareSerial.h>

const int bufferSize = 1024; // Kích thước bộ đệm tùy chỉnh
uint8_t buffer[bufferSize];
int bufferIndex = 0;
int numRows = 0;
int numCols = 0;
const int floatSize = sizeof(float);
void setup() {
  Serial.begin(115200);  // Đảm bảo baud rate khớp với baud rate của Python
}

void loop() {
  while (Serial.available() > 0) {
    uint8_t byteReceived = Serial.read();
    
    if (bufferIndex < bufferSize) {
      buffer[bufferIndex++] = byteReceived;
    } else {
      // Xử lý tình huống quá tải
      Serial.println("Buffer overflow");
      bufferIndex = 0; // Reset buffer index in case of overflow
    }
  }

  // if (bufferIndex > 0) {
  //   // Đã nhận dữ liệu
  //   Serial.println("Data received:");
  //   for (int i = 0; i < bufferIndex; ++i) {
  //     Serial.print(buffer[i]);
  //     Serial.print(" ");
  //   }
  //   Serial.println();
    
  //   // Reset chỉ số chỉ mục sau khi xử lý
  //   bufferIndex = 0;
  // }
  //  if (bufferIndex > 0) {
  //   Đã nhận dữ liệu
  //   Serial.println("Data received:");
  //   int numFloats = bufferIndex / sizeof(float);
  //   for (int i = 0; i < numFloats; ++i) {
  //     float value;
  //     memcpy(&value, &buffer[i * sizeof(float)], sizeof(float));
  //     Serial.print(value, 4);  // In với 4 chữ số thập phân
  //     Serial.print(" ");
  //   }
  //   Serial.println();
    
  //   Reset chỉ số chỉ mục sau khi xử lý
  //   bufferIndex = 0;
  // }
  if (bufferIndex >= 2 * sizeof(int)) {
    // Đã nhận đủ kích thước dữ liệu
    memcpy(&numRows, buffer, sizeof(int));
    memcpy(&numCols, buffer + sizeof(int), sizeof(int));

    int expectedSize = numRows * numCols * floatSize;

    if (bufferIndex >= (2 * sizeof(int) + expectedSize)) {
      Serial.println("Data received:");
      float data[numRows][numCols];
      for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numCols; ++col) {
          int index = (2 * sizeof(int)) + (row * numCols + col) * floatSize;
          float value;
          memcpy(&value, &buffer[index], floatSize);
          data[row][col] = value;
          Serial.print(value, 4);  // In với 4 chữ số thập phân
          Serial.print(" ");
        }
        Serial.println();
      }
      
      // Reset chỉ số chỉ mục sau khi xử lý
      bufferIndex = 0;
    }
  }
}
