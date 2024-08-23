# Project display object's point on matrix

### Using UART to communicate with each other

### Pinout ESP32-S3
![](/img/connector.jpg)

### Hardware

- ESP32-S3 N8R2 Dual Type-C
    ![ESP32-S3 N8R2 Dual Type-C](/img/esp32s3.jpg)
    

- Pinout
![](/img/esp32-s3_devkitc-1_pinlayout_v1.1.jpg)


```Cpp
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
```

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

#Game Space Invaders

x = 60 # X coordinate
y = 120 # Y coordinate (no action)
z = 1 # 1: fire bullet; 0 none fire
data = np.array([[60, 120,1]]).astype(np.float32)

```