from struct import pack

# Create a simple 16x16 32-bit icon with a blue background and white J shape.
width = 16
height = 16
pixels = []
for y in range(height):
    for x in range(width):
        if 4 <= x <= 11 and 3 <= y <= 4:
            pixels.append((255, 255, 255, 255))
        elif 6 <= x <= 7 and 5 <= y <= 12:
            pixels.append((255, 255, 255, 255))
        elif 5 <= x <= 8 and y == 13:
            pixels.append((255, 255, 255, 255))
        else:
            pixels.append((0, 120, 215, 255))

# ICO header
icon_data = bytearray()
icon_data += pack('<3H', 0, 1, 1)
icon_data += pack('<BBBBHHII', 16, 16, 0, 0, 1, 32, 0, 40 + 4 + 1024)
# BITMAPINFOHEADER
icon_data += pack('<IIIHHIIIIII', 40, width, height * 2, 1, 32, 0, width * height * 4, 0, 0, 0, 0)
# Pixel data (bottom-up)
for y in range(height - 1, -1, -1):
    for x in range(width):
        b, g, r, a = pixels[y * width + x]
        icon_data += pack('<BBBB', b, g, r, a)
# Mask data (all zeros)
icon_data += b'\x00' * (width * height // 8)

with open(r'c:\workspace\VisualCode\JSONTreeViewer\JSONTreeViewer.ico', 'wb') as f:
    f.write(icon_data)
