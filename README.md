# Project display object's point on matrix

# Swich branch to know configuration of each project 
- `Space Invader`
- `GameHungVat`
- `GameStickMan`

## Configuration
+ Connect ESP32 to PC via USB Type-C cable
+ port = "/dev/ttyACM0" (linux) "COM3" (Windows etc)
+ Baudrate : Speed rate (default: 230400)

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

# Data Frame to send
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

#Game Space Invader

x = 60 # X coordinate
y = 120 # Y coordinate (no action)
z = 1 # 1: fire bullet; 0 none fire
data = np.array([[60, 120,1]]).astype(np.float32)

```

## References
https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA
