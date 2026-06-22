#!/usr/bin/env python3
"""Generate a simple 16x16 Windows icon file"""
import struct

def create_icon():
    # ICO file header
    icon_data = bytearray()
    icon_data += struct.pack('<3H', 0, 1, 1)  # Reserved(0), Type(1=ICON), Count(1)
    
    # Icon directory entry
    width, height, color_count = 16, 16, 0
    icon_data += struct.pack('<BBBBHHII', 
                             width,           # Width
                             height,          # Height
                             color_count,     # Colors
                             0,               # Reserved
                             1,               # Planes
                             32,              # Bits per pixel
                             0,               # Bytes in image data (filled later)
                             6 + 16)          # Offset to image data (header size)
    
    # BMP info header
    bmp_size = 40 + (width * height * 4) + (width * height // 8)
    icon_data += struct.pack('<IIIHHIIIIII',
                             40,              # Header size
                             width,           # Width
                             height * 2,      # Height (doubled for BMP)
                             1,               # Planes
                             32,              # Bits per pixel
                             0,               # Compression (0=none)
                             width*height*4,  # Image size
                             0, 0, 0, 0)      # Resolution, color table entries
    
    # Pixel data: blue background with white "J"
    for y in range(height):
        for x in range(width):
            # White pixels for "J" shape
            if (4 <= x <= 11 and 3 <= y <= 4) or \
               (6 <= x <= 7 and 5 <= y <= 12) or \
               (5 <= x <= 8 and y == 13):
                icon_data += struct.pack('<BBBB', 255, 255, 255, 255)  # White BGRA
            else:
                icon_data += struct.pack('<BBBB', 215, 120, 0, 255)    # Blue BGRA
    
    # Mask data (1 bit per pixel, all zeros = transparent background)
    icon_data += b'\x00' * (width * height // 8)
    
    # Write file
    with open(r'C:\workspace\VisualCode\JSONTreeViewer\JSONTreeViewer.ico', 'wb') as f:
        f.write(icon_data)
    print('Icon file created successfully')

if __name__ == '__main__':
    create_icon()
