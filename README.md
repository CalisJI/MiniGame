# Project display object's point on matrix
### Data input example Serial Port

```python
import numpy as np
import serial
import struct

# Tạo mảng NumPy
n = 2
data = np.random.rand(n, 3).astype(np.float32)
# Kích thước của mảng
num_rows = data.shape[0]
num_cols = data.shape[1]
# Chuyển mảng thành chuỗi byte
data_bytes = data.tobytes()
# Đóng gói kích thước và dữ liệu
header = struct.pack('<II', num_rows, num_cols)  # <II cho hai số nguyên không dấu 32-bit
data_to_send = header + data_bytes
# Mở cổng serial
ser = serial.Serial('/dev/ttyUSB0', 115200)  # Thay 'COM3' bằng cổng serial của bạn
print(data)
# Gửi dữ liệu
ser.write(data_to_send)

# Đóng cổng serial
ser.close()

```

### Testing Game

```python
import numpy as np
import serial
import struct

# Tạo mảng NumPy
n = 2
# Kích thước của mảng

num_rows = 1
num_cols = 0

data = np.array([[60, 120,1]]).astype(np.float32)
num_rows = data.shape[0]
num_cols = data.shape[1]

# Chuyển mảng thành chuỗi byte
data_bytes = data.tobytes()
# Đóng gói kích thước và dữ liệu
header = struct.pack('<II', num_rows, num_cols)  # <II cho hai số nguyên không dấu 32-bit
data_to_send = header + data_bytes
# Mở cổng serial
ser = serial.Serial('/dev/ttyACM0', 115200*2)  # Thay 'COM3' bằng cổng serial của bạn
print(data)
# Gửi dữ liệu
ser.write(data_to_send)

# Đóng cổng serial
ser.close()

```

### Receive Data

```c++
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
```

# Data frame format
```python
# Game Hung Vat
x = 60 # X coordinate
y = 120 # Y coordinate (no action)
z = 1 # action (no action)
data = np.array([[60, 120,1]]).astype(np.float32)

# Game Stick Man
x = 60 # X coordinate
y = 120 # Y coordinate (no action)
z = 1 # 1: right shield; 2 left shield; 0 none sheild
data = np.array([[60, 120,1]]).astype(np.float32)

#Game Space Craft

x = 60 # X coordinate
y = 120 # Y coordinate (no action)
z = 1 # 1: fire bullet; 0 none fire
data = np.array([[60, 120,1]]).astype(np.float32)

```

## References
https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA