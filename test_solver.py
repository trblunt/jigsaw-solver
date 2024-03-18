import os
import csv
import time
from PIL import Image
import subprocess
import argparse
import random
import hashlib
from multiprocessing import Process, Queue, cpu_count, Lock
import logging

# Setup logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - [%(levelname)s] - %(processName)s - %(message)s')

def file_specific_seed(file_path, global_seed):
    """
    Generate a consistent, file-specific seed by combining the file path with a global seed.
    """
    # Encode the file path and convert the global seed to a string, then concatenate
    seed_str = f"{file_path}{global_seed}".encode('utf-8')
    # Use hashlib to generate a hash of the concatenated string
    hash_value = hashlib.sha256(seed_str).hexdigest()
    # Convert the hash to an integer seed
    file_seed = int(hash_value, 16) % (2**31)  # Use a 32-bit integer for the seed
    return file_seed

def process_images(queue, directory, piece_size, csv_file, seed, index, lock):
    generated_dir = f"generated/{index}/"
    os.makedirs(generated_dir, exist_ok=True)

    for filename in os.listdir(generated_dir):
        file_path = os.path.join(generated_dir, filename)
        try:
            if os.path.isfile(file_path) or os.path.islink(file_path):
                os.unlink(file_path)
        except Exception as e:
            logging.error(f"Error removing {file_path}: {e}")

    while True:
        relative_path = queue.get()
        if relative_path is None:  # Shutdown signal
            break

        file = os.path.join(directory, relative_path)
        try:
            try:
                img = Image.open(file).convert('RGB')
            except IOError:
                continue

            img = img.resize((224, 224), resample=Image.Resampling.LANCZOS)
            temp_img_name = "temp_" + os.path.splitext(os.path.basename(file))[0] + ".jpg"
            temp_img_path = os.path.join(generated_dir, temp_img_name)
            img.save(temp_img_path, format="JPEG")

            n = 224 // piece_size
            start_time = time.time()

            file_seed = file_specific_seed(file, seed)

            generate_cmd = ["./generate_pieces", temp_img_path, str(piece_size), generated_dir, str(file_seed)]
            subprocess.run(generate_cmd, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)

            solver_cmd = ["./solver", str(n), generated_dir]
            process = subprocess.run(solver_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            ncs_value = process.stdout.strip().split("NCS: ")[-1]
            end_time = time.time()
            execution_time = end_time - start_time

            with lock:
                logging.info(f"{relative_path} was solved in {execution_time:.2f} seconds with accuracy of {float(ncs_value)*100:.2f}%.")
                with open(csv_file, "a", newline='') as file_handle:
                    writer = csv.writer(file_handle)
                    writer.writerow([relative_path, ncs_value, end_time - start_time])

            os.remove(temp_img_path)
        except Exception as e:
            logging.error(f"Error processing {file}: {e}")
        except KeyboardInterrupt:
            break;

# Function to get the names of already processed files
def get_processed_files(csv_file):
    if not os.path.exists(csv_file):
        return set()
    with open(csv_file, mode='r', newline='') as file:
        reader = csv.reader(file)
        return {rows[0] for rows in reader}

def producer(directory, seed, queue, csv_file="results.csv"):
    processed_files = get_processed_files(csv_file)
    random.seed(seed)
    all_files = []
    for root, _, files in os.walk(directory):
        for file in files:
            relative_path = os.path.relpath(os.path.join(root, file), start=directory)
            all_files.append(relative_path)
    random.shuffle(all_files)
    for file in all_files:
        if file not in processed_files:
            queue.put(file)
    for _ in range(cpu_count()):
        queue.put(None)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Process some images with multiprocessing support.")
    parser.add_argument('--directory', type=str, required=True, help='Directory to process')
    parser.add_argument('--piece_size', type=int, default=16, help='Side length of the puzzle piece')
    parser.add_argument('--csv', type=str, default='results.csv', help='CSV file to store results')
    parser.add_argument('--seed', type=int, default=42, help='Seed for random directory walk')
    parser.add_argument('--threads', type=int, default=cpu_count(), help='Number of concurrent threads')

    args = parser.parse_args()

    queue = Queue()
    lock = Lock()
    processes = []

    try:
        # Start the producer process
        producer_process = Process(target=producer, args=(args.directory, args.seed, queue, args.csv))
        producer_process.start()

        # Start consumer processes
        for index in range(args.threads):
            p = Process(target=process_images, args=(queue, args.directory, args.piece_size, args.csv, args.seed, index, lock))
            processes.append(p)
            p.start()

        producer_process.join()

        for p in processes:
            p.join()

    except KeyboardInterrupt:
        producer_process.terminate()
        producer_process.join()
        for p in processes:
            p.terminate()
            p.join()
        logging.info("Cleanup complete. Exiting now.")
