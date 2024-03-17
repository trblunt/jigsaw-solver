import os
import csv
import time
from PIL import Image
from subprocess import Popen, PIPE
import argparse
import signal
import subprocess
import sys

# Setup graceful termination
terminate = False

def signal_handler(sig, frame):
    global terminate
    terminate = True

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)

# Function to check if an image is already processed
def is_processed(filename, processed_files):
    return filename in processed_files

# Function to get the names of already processed files
def get_processed_files(csv_file):
    if not os.path.exists(csv_file):
        return set()
    with open(csv_file, mode='r', newline='') as file:
        reader = csv.reader(file)
        return {rows[0] for rows in reader}

# Function to process images
def process_images(directory, piece_size, csv_file):
    processed_files = get_processed_files(csv_file)

    for root, dirs, files in os.walk(directory):
        for file in files:
            if terminate:
                return
            if not is_processed(file, processed_files):
                try:
                    # Attempt to open the image
                    img_path = os.path.join(root, file)
                    try:
                        img = Image.open(img_path).convert('RGB')
                    except IOError:
                        continue  # Not an image file

                    # Image transformations
                    img = img.resize((224, 224), resample=Image.Resampling.LANCZOS)
                    
                    # Save transformed image to temp location in JPG format
                    temp_img_path = os.path.join(root, "temp_" + os.path.splitext(file)[0] + ".jpg")
                    img.save(temp_img_path, format="JPEG")

                    # Calculate n
                    n = 224 // piece_size

                    # Run generate_pieces
                    start_time = time.time()
                    # Run generate_pieces with output redirected
                    generate_cmd = ["./generate_pieces", temp_img_path, str(piece_size)]
                    with open(os.devnull, 'w') as FNULL:
                        subprocess.run(generate_cmd, stdout=FNULL, stderr=subprocess.STDOUT)

                    # Calculate n and run solver with captured output
                    n = 224 // piece_size
                    solver_cmd = ["./solver", str(n)]
                    process = subprocess.run(solver_cmd, stdout=PIPE, stderr=PIPE, text=True)
                    ncs_value = process.stdout.strip().split("NCS: ")[-1]

                    if terminate:
                        return

                    # Execution time and custom summary message
                    end_time = time.time()
                    execution_time = end_time - start_time
                    print(f"{file} was solved in {execution_time:.2f} seconds with accuracy of {float(ncs_value)*100:.2f}%.")

                    # Write to CSV
                    with open(csv_file, mode='a', newline='') as file_handle:
                        writer = csv.writer(file_handle)
                        writer.writerow([file, ncs_value, execution_time])

                    # Cleanup temp file
                    os.remove(temp_img_path)
                    
                except Exception as e:
                    print(f"Error processing {file}: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Process some images.")
    parser.add_argument('--directory', type=str, required=True, help='Directory to process')
    parser.add_argument('--piece_size', type=int, default=16, help='Side length of the puzzle piece')
    parser.add_argument('--csv', type=str, default='results.csv', help='CSV file to store results')
    args = parser.parse_args()

    process_images(args.directory, args.piece_size, args.csv)
