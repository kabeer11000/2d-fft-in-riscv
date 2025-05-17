import sys
import os
import subprocess
import math
from PIL import Image

# --- Configuration ---
# Name of your input image file (should be in the same directory as this script)
INPUT_IMAGE = "input_image.jpg" # Or .png

# Paths relative to the script's directory
SOURCE_DIR = "source/"
BUILD_DIR = "build/"
RAW_DATA_HEADER = os.path.join(SOURCE_DIR, "image_data.h")
C_SOURCE_FILE = os.path.join(SOURCE_DIR, "main.c")
# File to provide dummy bare-metal symbols (__errno)
BARE_METAL_SYMBOLS_FILE = os.path.join(SOURCE_DIR, "bare_metal_symbols.c")


# Base name for the compiled executable
EXECUTABLE_NAME = "fft_image"
# Full path to the output executable
OUTPUT_EXECUTABLE_PATH = os.path.join(BUILD_DIR, EXECUTABLE_NAME)


# Paths to your RISC-V toolchain and QEMU
# Adjust these if they are not in your system's PATH
QEMU_PATH = "qemu-system-riscv64"
COMPILER_PATH = "riscv64-unknown-elf-gcc"

# Compiler flags for bare-metal RISC-V with vector extensions
# -O2: Optimization level 2
# -Wall: Enable all warnings
# -nostdlib: Do not link the standard C library
# -march=rv64gcv: Specify architecture (rv64, general-purpose, compressed, vector)
# -mabi=lp64d: Specify ABI (LP64 double-precision float)
# -o ...: Output file name
# -lm: Link the math library (necessary for sinf/cosf/sqrtf)
# -Wl,--no-relax: Linker flag, --no-relax can sometimes be needed for bare-metal/linking order
COMPILER_FLAGS = "-O2 -Wall -nostdlib -march=rv64gv -T link.ld -mabi=lp64d -o {} {} {} -lm -Wl,--no-relax".format(OUTPUT_EXECUTABLE_PATH, C_SOURCE_FILE, BARE_METAL_SYMBOLS_FILE)

# QEMU flags for the 'virt' machine with vector extension enabled
# -machine virt: Use the generic 'virt' machine
# -cpu rv64,v=true: Use a RISC-V 64-bit CPU with the vector extension enabled
# -bios none: Do not load a BIOS (run our kernel directly)
# -kernel ...: Load our compiled executable as the kernel
# -nographic: Disable graphical output, route console output to stdout
QEMU_FLAGS = "-machine virt -cpu rv64,v=true -bios none -kernel {} -nographic".format(OUTPUT_EXECUTABLE_PATH)

# Output file names for processed data (saved on the host)
OUTPUT_RAW_MAGNITUDE = "output_fft_magnitude.raw"
OUTPUT_MAGNITUDE_IMAGE = "output_fft_magnitude.png"

# --- Target FFT size ---
# Choose a smaller power of 2 for faster processing in the demo.
# The image will be resized to this square dimension.
# Common sizes: 32, 64, 128, 256.
TARGET_FFT_SIZE = 128 # <--- Set your desired size here (must be power of 2)
# ------------------------------------------


# --- Image Loading and Preprocessing ---
def load_and_prepare_image(image_path, target_size):
    """Loads an image, converts to grayscale, resizes to target_size x target_size."""
    try:
        # Open image and convert to grayscale
        img = Image.open(image_path).convert('L')
    except FileNotFoundError:
        print(f"Error: Input image '{image_path}' not found.")
        sys.exit(1)
    except Exception as e:
        print(f"Error opening or processing image '{image_path}': {e}")
        sys.exit(1)

    # Resize to the target power of 2 size
    # This will downscale or upscale the image
    print(f"Original size: {img.size[0]}x{img.size[1]}, Resizing to target size: {target_size}x{target_size}")
    img_resized = img.resize((target_size, target_size))

    # Get pixel data as a flat list of floats (0.0 to 255.0)
    # FFT typically works on float data
    pixel_data = list(img_resized.getdata())
    float_data = [float(p) for p in pixel_data]

    return target_size, target_size, float_data


# --- Generate C Header with Image Data ---
def generate_c_header(width, height, data, output_file):
    """Generates a C header file containing the image data as a float array."""
    print(f"Generating {output_file}...")
    try:
        with open(output_file, "w") as f:
            f.write("#ifndef IMAGE_DATA_H\n")
            f.write("#define IMAGE_DATA_H\n\n")
            f.write(f"#define IMAGE_WIDTH {width}\n")
            f.write(f"#define IMAGE_HEIGHT {height}\n")
            f.write("const float input_image_data[] = {\n")
            for i, val in enumerate(data):
                f.write(f"{val}f, ")
                if (i + 1) % 10 == 0: # Format for readability
                    f.write("\n")
            # Remove trailing comma if exists
            if data:
                 f.seek(f.tell() - 2, os.SEEK_SET) # Move back past ", "
                 f.truncate()
            f.write("\n};\n\n")
            f.write("#endif // IMAGE_DATA_H\n")
        print("Header generated.")
    except Exception as e:
        print(f"Error writing header file '{output_file}': {e}")
        sys.exit(1)


# --- Compile C Code ---
def compile_c_code():
    """Compiles the C source file using the specified toolchain and flags."""
    print(f"Compiling C code: {COMPILER_PATH} {COMPILER_FLAGS}")
    try:
        # Use shell=True cautiously, but necessary here to easily use string flags
        # check=True raises CalledProcessError if the command fails
        subprocess.run(f"{COMPILER_PATH} {COMPILER_FLAGS}", shell=True, check=True, text=True)
        print("Compilation successful.")
    except subprocess.CalledProcessError as e:
        print(f"Error during compilation: {e}")
        print(f"Compiler stdout:\n{e.stdout}")
        print(f"Compiler stderr:\n{e.stderr}")
        sys.exit(1)
    except FileNotFoundError:
         print(f"Error: Compiler '{COMPILER_PATH}' not found. Is RISC-V toolchain installed and in PATH?")
         sys.exit(1)


# --- Run QEMU and Capture Output ---
def run_qemu_and_capture_output():
    """Runs the compiled executable in QEMU and captures its standard output."""
    print(f"Running QEMU: {QEMU_PATH} {QEMU_FLAGS}")
    try:
        # Capture output as text
        result = subprocess.run(f"{QEMU_PATH} {QEMU_FLAGS}", shell=True, capture_output=True, text=True, check=True)
        print("QEMU execution finished.")
        # Return the captured standard output string
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Error during QEMU execution: {e}")
        print(f"QEMU stdout:\n{e.stdout}")
        print(f"QEMU stderr:\n{e.stderr}")
        # QEMU often exits with non-zero status on errors or crashes in the guest
        sys.exit(1)
    except FileNotFoundError:
         print(f"Error: QEMU executable '{QEMU_PATH}' not found. Is QEMU installed and in PATH?")
         sys.exit(1)


# --- Parse QEMU Output and Save Results ---
def parse_output_and_save(output_str, width, height):
    """Parses the output string from QEMU and saves the FFT magnitude data."""
    print("Parsing QEMU output...")
    # The C code prints comma-separated floats, possibly with newlines
    # Replace newlines with commas and split by commas, filtering empty strings
    values = [float(x.strip()) for x in output_str.replace('\n', ',').split(',') if x.strip()]

    expected_count = width * height
    if len(values) != expected_count:
        print(f"Warning: Expected {expected_count} output values ({width}x{height}), but got {len(values)}. Output data may be incomplete or corrupted.")
        # Trim or pad values if necessary for saving, depending on desired behavior
        # For visualization, we'll use the values we got, potentially resulting in a partial image
        values = values[:expected_count] # Take only up to the expected count


    print(f"Parsed {len(values)} floating point values.")

    if not values:
        print("No numerical data parsed from QEMU output. Cannot save results.")
        return

    # Save raw magnitude data as binary floats
    print(f"Saving raw magnitude data to {OUTPUT_RAW_MAGNITUDE}...")
    try:
        with open(OUTPUT_RAW_MAGNITUDE, "wb") as f:
            import struct
            for val in values:
                # Use little-endian float format '<f' for standard compatibility
                f.write(struct.pack('<f', val))
        print("Raw data saved.")
    except Exception as e:
         print(f"Error saving raw data: {e}")


    # Attempt to save as a grayscale image for visualization
    print(f"Saving magnitude visualization to {OUTPUT_MAGNITUDE_IMAGE}...")
    try:
        # Normalize magnitude values for display (log scale is common for FFT visualization)
        # Avoid log(0) by using log1p (log(1 + x))
        normalized_values = [math.log1p(abs(v)) for v in values] # abs() handles potential negative zeros

        # Scale to 0-255 for grayscale image
        max_val = max(normalized_values) if normalized_values else 1.0 # Avoid division by zero
        # Scale to 0-255. Clamp to 255 for safety, though scaling should handle it.
        scaled_values = [min(255, max(0, int((v / max_val) * 255))) for v in normalized_values]

        # Pad scaled_values if we got fewer values than expected
        if len(scaled_values) < expected_count:
             scaled_values.extend([0] * (expected_count - len(scaled_values)))


        # Create a new grayscale image
        img_out = Image.new('L', (width, height))
        img_out.putdata(scaled_values)

        # Optional: Shift the zero-frequency component (DC) to the center for better visualization
        # This is a standard step when visualizing FFT magnitude spectrums
        img_shifted = Image.new('L', (width, height))
        half_width = width // 2
        half_height = height // 2

        # Copy quadrants: Top-Left -> Bottom-Right, Top-Right -> Bottom-Left, etc.
        img_shifted.paste(img_out.crop((half_width, half_height, width, height)), (0, 0)) # Bottom-Right -> Top-Left
        img_shifted.paste(img_out.crop((0, half_height, half_width, height)), (half_width, 0)) # Bottom-Left -> Top-Right
        img_shifted.paste(img_out.crop((half_width, 0, width, half_height)), (0, half_height)) # Top-Right -> Bottom-Left
        img_shifted.paste(img_out.crop((0, 0, half_width, half_height)), (half_width, half_height)) # Top-Left -> Bottom-Right


        img_shifted.save(OUTPUT_MAGNITUDE_IMAGE)
        print("Magnitude image visualization saved.")
    except Exception as e:
        print(f"Error saving magnitude image: {e}")


# --- Main Execution Flow ---
if __name__ == "__main__":
    # Create necessary directories if they don't exist
    os.makedirs(SOURCE_DIR, exist_ok=True)
    os.makedirs(BUILD_DIR, exist_ok=True)

    # 1. Load and prepare image data using the target size
    img_width, img_height, raw_pixel_data = load_and_prepare_image(INPUT_IMAGE, TARGET_FFT_SIZE)

    # 2. Generate C header file (now goes into source directory)
    generate_c_header(img_width, img_height, raw_pixel_data, RAW_DATA_HEADER)

    # 3. Compile C code (including the new symbols file)
    compile_c_code()

    # 4. Run QEMU and capture output
    qemu_output = run_qemu_and_capture_output()

    # 5. Parse output and save results using the target size
    parse_output_and_save(qemu_output, img_width, img_height)

    print("\nProcess completed.")
    print(f"Input image: {INPUT_IMAGE}")
    print(f"Processed size: {img_width}x{img_height}")
    print(f"Output raw magnitude data: {OUTPUT_RAW_MAGNITUDE}")
    print(f"Output magnitude visualization: {OUTPUT_MAGNITUDE_IMAGE}")