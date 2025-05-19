from PIL import Image
import numpy as np
import os

def convert_image_to_header(jpg_path, output_header):
    # Open and convert to grayscale
    img = Image.open(jpg_path).convert('L')
    width, height = img.size
    
    # Convert to numpy array
    img_array = np.array(img)
    
    # Create header file
    with open(output_header, 'w') as f:
        f.write('#ifndef IMAGE_DATA_H\n')
        f.write('#define IMAGE_DATA_H\n\n')
        f.write(f'#define IMAGE_WIDTH {width}\n')
        f.write(f'#define IMAGE_HEIGHT {height}\n\n')
        f.write('const unsigned char image_data[] = {\n')
        
        # Write data in rows of 16 bytes
        for i in range(0, len(img_array), 16):
            row = img_array[i:i+16]
            f.write('    ' + ', '.join(f'0x{x:02x}' for x in row) + ',\n')
        
        f.write('};\n\n')
        f.write('#endif // IMAGE_DATA_H\n')

if __name__ == '__main__':
    convert_image_to_header('input_image.jpg', 'source/image_data.h') 