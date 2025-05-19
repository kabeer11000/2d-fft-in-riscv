from PIL import Image
import numpy as np
import os

def raw_to_png(raw_path, png_path, width, height):
    # Read raw data
    with open(raw_path, 'rb') as f:
        data = np.frombuffer(f.read(), dtype=np.uint8)
    
    # Reshape to image dimensions
    img_array = data.reshape((height, width))
    
    # Create and save image
    img = Image.fromarray(img_array)
    img.save(png_path)

if __name__ == '__main__':
    # Convert both original and compressed images
    raw_to_png('original.raw', 'original.png', IMAGE_WIDTH, IMAGE_HEIGHT)
    raw_to_png('compressed.raw', 'compressed.png', IMAGE_WIDTH, IMAGE_HEIGHT) 
 