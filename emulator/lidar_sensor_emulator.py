import socket
import csv
import time
from pathlib import Path

UDP_IP = "127.0.0.1"
UDP_PORT = 25000
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

BASE_DIR = Path(__file__).resolve().parent.parent
filename = BASE_DIR / "data" / "lidar_data.csv"

current_time = None
frame_lines = []
with open(filename, "r") as file:
    reader = csv.reader(file)
    next(reader)  # skip header
    for row in reader:
        t, rayId, X, Y, Z, intensity = row
        line = f"{t};{rayId};{X};{Y};{Z};{intensity}"
        # Grouping by timestamp
        if current_time is None:
            current_time = t
        if t != current_time:
            # Sending the previous frame if there is difference in time stamp change
            message = "\n".join(frame_lines) + "\n"
            sock.sendto(message.encode("utf-8"), (UDP_IP, UDP_PORT))
            print(f"Sent frame at time {current_time}")
            # Resetting the array for the next frame
            frame_lines = []
            current_time = t
            # Optional delay to just to visualize in matlab display. can be commented out to get the data directly through to the to workspace block
            time.sleep(0.1)
        frame_lines.append(line)
    # Sending the last frame
    if frame_lines:
        message = "\n".join(frame_lines) + "\n"
        sock.sendto(message.encode("utf-8"), (UDP_IP, UDP_PORT))
        print(f"Sent last frame at time {current_time}")
   