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
