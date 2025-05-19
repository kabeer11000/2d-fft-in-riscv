import sys
import os
import subprocess
import math
from PIL import Image
import shutil # Import shutil for cleanup (alternative to make clean)

# --- Configuration ---
# Name of your input image file (should be in the same directory as this script)
INPUT_IMAGE = "input_image.jpg" # Or .png

# Paths relative to the script's directory
SOURCE_DIR = "source/"
BUILD_DIR = "build/"
RAW_DATA_HEADER = os.path.join(SOURCE_DIR, "image_data.h") # Header goes into source/ now

# The command to invoke your Makefile
MAKE_COMMAND = "make"

# Target FFT size
# Choose a smaller power of 2 for faster processing in the demo.
# The image will be resized to this square dimension.
# Common sizes: 32, 64, 128, 256.
TARGET_FFT_SIZE = 128 # <--- Set your desired size here (must be power of 2)
# ------------------------------------------

# Output file names for processed data (saved on the host)
OUTPUT_RAW_MAGNITUDE = "output_fft_magnitude.raw"
OUTPUT_MAGNITUDE_IMAGE = "output_fft_magnitude.png"


# --- Helper Functions ---
def run_command(command, description):
    """Runs a shell command and checks for errors."""
    print(f"--- {description} ---")
    print(f"Executing: {command}")
    try:
        # Use shell=True for simplicity with command strings, but be mindful of security if inputs are untrusted.
        # check=True raises CalledProcessError if the command returns a non-zero exit code.
        # capture_output=True captures stdout and stderr.
        # text=True decodes stdout/stderr as text.
        result = subprocess.run(command, shell=True, check=True, capture_output=True, text=True)
        print(f"--- {description} successful ---")
        # Return stdout for commands like 'make run'
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"--- Error during {description} ---")
        print(f"Command failed with exit code {e.returncode}")
        print(f"Stdout:\n{e.stdout}")
        print(f"Stderr:\n{e.stderr}")
        sys.exit(1) # Exit the script on failure
    except FileNotFoundError:
         print(f"Error: Command '{command.split()[0]}' not found. Is it in your system's PATH?")
         sys.exit(1)


# --- Image Loading and Preprocessing ---
def load_and_prepare_image(image_path, target_size):
    """Loads an image, converts to grayscale, resizes to target_size x target_size."""
    print(f"--- Loading and Preparing Image ---")
    try:
        # Open image and convert to grayscale
        img = Image.open(image_path).convert('L')
    except FileNotFoundError:
        print(f"Error: Input image '{image_path}' not found.")
        sys.exit(1)
    except Exception as e:
        print(f"Error opening or processing image '{image_path}': {e}")
        sys.exit(1)

    # Check if target size is a power of 2
    if not (target_size > 0 and (target_size & (target_size - 1)) == 0):
         print(f"Error: TARGET_FFT_SIZE ({target_size}) must be a power of 2.")
         sys.exit(1)


    # Resize to the target power of 2 size
    # This will downscale or upscale the image
    print(f"Original size: {img.size[0]}x{img.size[1]}, Resizing to target size: {target_size}x{target_size}")
    img_resized = img.resize((target_size, target_size))

    # Get pixel data as a flat list of floats (0.0 to 255.0)
    # FFT typically works on float data
    pixel_data = list(img_resized.getdata())
    float_data = [float(p) for p in pixel_data]

    print(f"Image prepared: {target_size}x{target_size} pixels.")
    return target_size, target_size, float_data


# --- Generate C Header with Image Data ---
def generate_c_header(width, height, data, output_file):
    """Generates a C header file containing the image data as a float array."""
    print(f"--- Generating C Header: {output_file} ---")
    try:
        with open(output_file, "w") as f:
            f.write("#ifndef IMAGE_DATA_H\n")
            f.write("#define IMAGE_DATA_H\n\n")
            f.write(f"#define IMAGE_WIDTH {width}\n")
            f.write(f"#define IMAGE_HEIGHT {height}\n")
            f.write("const float input_image_data[] = {\n")
            for i, val in enumerate(data):
                f.write(f"{val}f, ")
                if (i + 1) % 10 == 0 and (i + 1) < len(data): # Format for readability, avoid trailing comma
                    f.write("\n")
            # Remove trailing comma and space from the very last element if it exists
            if data:
                 f.seek(f.tell() - 2, os.SEEK_SET) # Move back past ", "
                 f.truncate()
            f.write("\n};\n\n")
            f.write("#endif // IMAGE_DATA_H\n")
        print("Header generated successfully.")
    except Exception as e:
        print(f"Error writing header file '{output_file}': {e}")
        sys.exit(1)


# --- Parse QEMU Output and Save Results ---
def parse_output_and_save(output_str, width, height):
    """Parses the output string from QEMU and saves the FFT magnitude data."""
    print("--- Parsing QEMU Output ---")
    # The C code prints comma-separated floats, possibly with newlines
    # Replace newlines with commas and split by commas, filtering empty strings
    # The C code prints real and imaginary parts interleaved: r1, i1, r2, i2, ...
    # The Python script parses all values into a single list
    values = [float(x.strip()) for x in output_str.replace('\n', ',').split(',') if x.strip()]

    expected_float_count = width * height * 2 # Each complex number is 2 floats (real, imag)

    if len(values) != expected_float_count:
        print(f"Warning: Expected {expected_float_count} output floats ({width}x{height} complex numbers), but got {len(values)}. Output data may be incomplete or corrupted.")
        # Trim or pad values if necessary for saving/visualization
        values = values[:expected_float_count] # Take only up to the expected count


    print(f"Parsed {len(values)} floating point values ({len(values)//2} complex numbers).")

    if len(values) < 2: # Need at least one complex number (2 floats)
        print("No sufficient numerical data parsed from QEMU output. Cannot save results.")
        return

    # The values are interleaved: real0, imag0, real1, imag1, ...
    # Extract magnitudes: magnitude = sqrt(real^2 + imag^2)
    magnitude_values = []
    # Ensure we process pairs, even if output was truncated
    for i in range(0, len(values) - 1, 2):
        real = values[i]
        imag = values[i+1]
        magnitude = math.sqrt(real*real + imag*imag)
        magnitude_values.append(magnitude)

    print(f"Calculated {len(magnitude_values)} magnitude values.")


    # Save raw magnitude data as binary floats
    print(f"Saving raw magnitude data to {OUTPUT_RAW_MAGNITUDE}...")
    try:
        with open(OUTPUT_RAW_MAGNITUDE, "wb") as f:
            import struct
            for val in magnitude_values:
                # Use little-endian float format '<f' for standard compatibility
                f.write(struct.pack('<f', val))
        print("Raw data saved.")
    except Exception as e:
         print(f"Error saving raw data: {e}")


    # Attempt to save as a grayscale image for visualization
    # Use the *original* dimensions for saving the image visualization
    output_image_width = width
    output_image_height = height

    # Pad or truncate magnitude_values to match the output image size if necessary
    expected_magnitude_count = output_image_width * output_image_height
    if len(magnitude_values) < expected_magnitude_count:
        print(f"Warning: Padding magnitude data ({len(magnitude_values)} < {expected_magnitude_count}) for image visualization.")
        magnitude_values.extend([0.0] * (expected_magnitude_count - len(magnitude_values)))
    elif len(magnitude_values) > expected_magnitude_count:
        print(f"Warning: Truncating magnitude data ({len(magnitude_values)} > {expected_magnitude_count}) for image visualization.")
        magnitude_values = magnitude_values[:expected_magnitude_count]


    print(f"Saving magnitude visualization to {OUTPUT_MAGNITUDE_IMAGE}...")
    try:
        # Normalize magnitude values for display (log scale is common for FFT visualization)
        # Avoid log(0) by using log1p (log(1 + x))
        # Also adding a small epsilon just in case abs(v) is very close to zero after IFFT
        normalized_values = [math.log1p(v + 1e-9) for v in magnitude_values] # Use log1p(magnitude)

        # Scale to 0-255 for grayscale image
        max_val = max(normalized_values) if normalized_values else 1.0 # Avoid division by zero
        # Scale to 0-255. Clamp to 255 for safety, though scaling should handle it.
        scaled_values = [min(255, max(0, int((v / max_val) * 255))) for v in normalized_values]


        # Create a new grayscale image
        img_out = Image.new('L', (output_image_width, output_image_height))
        img_out.putdata(scaled_values)

        # Optional: Shift the zero-frequency component (DC) to the center for better visualization
        # This is a standard step when visualizing FFT magnitude spectrums
        img_shifted = Image.new('L', (output_image_width, output_image_height))
        half_width = output_image_width // 2
        half_height = output_image_height // 2

        # Copy quadrants: Top-Left -> Bottom-Right, Top-Right -> Bottom-Left, etc.
        # Handle cases where width or height is odd (though we require power of 2, so always even)
        img_shifted.paste(img_out.crop((half_width, half_height, output_image_width, output_image_height)), (0, 0)) # Bottom-Right -> Top-Left
        img_shifted.paste(img_out.crop((0, half_height, half_width, output_image_height)), (half_width, 0)) # Bottom-Left -> Top-Right
        img_shifted.paste(img_out.crop((half_width, 0, output_image_width, half_height)), (0, half_height)) # Top-Right -> Bottom-Left
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

    # Optional: Clean previous build
    # run_command(f"{MAKE_COMMAND} clean", "Cleaning build directory")
    # shutil.rmtree(BUILD_DIR, ignore_errors=True) # Alternative Python cleanup

    # 1. Load and prepare image data using the target size
    img_width, img_height, raw_pixel_data = load_and_prepare_image(INPUT_IMAGE, TARGET_FFT_SIZE)

    # 2. Generate C header file (now goes into source directory)
    generate_c_header(img_width, img_height, raw_pixel_data, RAW_DATA_HEADER)

    # 3. Compile C code using make
    run_command(MAKE_COMMAND, "Compiling with Makefile")

    # 4. Run QEMU using make target and capture output
    qemu_output = run_command(f"{MAKE_COMMAND} run", "Running QEMU via Makefile")

    # 5. Parse output and save results using the target size
    parse_output_and_save(qemu_output, img_width, img_height)

    print("\nProcess completed successfully.")
    print(f"Input image: {INPUT_IMAGE}")
    print(f"Processed size: {img_width}x{img_height}")
    print(f"Output raw magnitude data: {OUTPUT_RAW_MAGNITUDE}")
    print(f"Output magnitude visualization: {OUTPUT_MAGNITUDE_IMAGE}")