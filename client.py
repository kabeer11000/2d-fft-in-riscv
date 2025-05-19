import requests
import numpy as np
from PIL import Image

# Create a dummy grayscale image (e.g., 256x256)
width, height = 256, 256
# Example: A simple gradient image
img_data = np.arange(width * height, dtype=np.uint8).reshape((height, width)) % 255
# Or load a real image and convert to grayscale
# img = Image.open("your_image.png").convert("L") # Convert to grayscale
# img_data = np.array(img, dtype=np.uint8)

# Flatten the image data to send as raw bytes
raw_image_bytes = img_data.tobytes()

server_url = "http://<RISCV_TARGET_IP>:8080/compress"

try:
    response = requests.post(server_url, data=raw_image_bytes, 
                             headers={'Content-Type': 'application/octet-stream'})

    if response.status_code == 200:
        print(f"Compression successful. Received {len(response.content)} bytes of compressed data.")
        # You would then need a decompression client to reverse the process
        # For this simplified example, the client would need to know the
        # quantization factor and the very basic encoding.
        # Example: If you just sent raw quantized shorts:
        # received_shorts = np.frombuffer(response.content, dtype=np.int16)
        # print(f"Decompressed shorts shape: {received_shorts.shape}")
    else:
        print(f"Error: {response.status_code} - {response.text}")
except requests.exceptions.ConnectionError as e:
    print(f"Could not connect to the server: {e}")
except Exception as e:
    print(f"An error occurred: {e}")