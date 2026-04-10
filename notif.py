import subprocess
import time

time.sleep(5)

try:
    subprocess.run(["notify-send", "-u", "critical", "ACTION REQUIRED: GRADUATION", "COMPLETED! "*50], check=True)
except FileNotFoundError:
    print("Error: notify-send not found. Install libnotify.")

