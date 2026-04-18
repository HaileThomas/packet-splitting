import subprocess
import re
import csv
from datetime import datetime

TARGET_SOCKET = 0
TARGET_STACK  = 2
TARGET_IFACE  = "enp59s0f0np0"
OUTPUT_CSV    = "monitor_output.csv"

def read_rx():
    stdout, _ = subprocess.Popen(
        ["ethtool", "-S", TARGET_IFACE],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True
    ).communicate()
    match = re.search(r'rx_vport_unicast_bytes:\s+(\d+)', stdout)
    return int(match.group(1)) if match else None

def monitor():
    pattern = re.compile(rf"Socket{TARGET_SOCKET}.*IIO Stack {TARGET_STACK}.*Part0")
    prev_rx = None
    buffer = None
    first_interval = True

    try:
        process = subprocess.Popen(
            ["pcm-iio", "1", "-csv"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1
        )

        print(f"[PCM] Monitoring Socket {TARGET_SOCKET}, Stack {TARGET_STACK}... Writing to {OUTPUT_CSV}")

        with open(OUTPUT_CSV, "w", newline="") as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(["timestamp", "ib_write", "ib_read", "rx_delta"])

            for line in process.stdout:
                if line.startswith("Socket,Name"):
                    if not first_interval and buffer is not None:
                        curr_rx = read_rx()
                        rx_delta = (curr_rx - prev_rx) if (curr_rx is not None and prev_rx is not None) else None
                        prev_rx = curr_rx
                        writer.writerow([datetime.now().isoformat(), buffer[0], buffer[1], rx_delta])
                        csvfile.flush()

                    buffer = None

                    if first_interval:
                        prev_rx = read_rx()
                        first_interval = False
                    continue

                if pattern.search(line):
                    fields = line.strip().split(',')
                    if len(fields) >= 7:
                        ib_write = int(fields[3]) if fields[3] else 0
                        ib_read  = int(fields[4]) if fields[4] else 0
                    else:
                        ib_write = ib_read = 0
                    buffer = (ib_write, ib_read)

    except FileNotFoundError:
        print("Error: 'pcm-iio' not found. Make sure Intel PCM is installed and in your PATH.")
    except KeyboardInterrupt:
        print(f"\nMonitoring stopped. Data saved to {OUTPUT_CSV}")
    finally:
        if 'process' in locals() and process.poll() is None:
            process.terminate()

if __name__ == "__main__":
    monitor()