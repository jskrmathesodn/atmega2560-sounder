import serial
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors


# Settings
MAX_RANGE_CM = 180  # 1.8 meters
HISTORY = 100 # Number of columns (time steps)
COM_PORT = 'COM5'
BAUD_RATE = 115200

ser = serial.Serial(COM_PORT, BAUD_RATE)

# Create 2D grid - rows = distance, cols = time
grid = np.zeros((MAX_RANGE_CM, HISTORY))

# Setup plot
fig, ax = plt.subplots(figsize=(12, 6))
img = ax.imshow(
    grid,
    aspect='auto',
    origin='upper',
    cmap='ocean', # Blue/green fish finder colour
    vmin=0,
    vmax=1,
    interpolation='nearest'
)

# Colourbar
cbar = plt.colorbar(img, ax=ax)
cbar.set_label('Object Detected')

ax.set_title('Depth Sounder Visualization')
ax.set_xlabel('Time (samples)')
ax.set_ylabel('Distance (cm)')

# Y axis labels in cm
ax.set_yticks([0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180])
ax.set_yticklabels(['0', '10', '20', '30', '40', '50', '60', '70', '80', '90', '100', '110', '120', '130', '140', '150', '160', '170', '180'])

# X axis labels
ax.set_xticks([0, 25, 50, 75, 99])
ax.set_xticklabels(['-100', '-75', '-50', '-25', 'now'])

plt.tight_layout()
plt.ion()
plt.show()

while True:
    ser.reset_input_buffer()  # Clear old data before reading
    raw = ser.readline().decode('utf-8').strip()
    
    if raw == "Out of range":
        # No object detected, don't update grid
        pass
    elif raw.isdigit():
        print(f"Distance: {raw} cm")
        distance = int(raw)

        # Scroll grid left
        grid = np.roll(grid, -1, axis=1)
        grid[:, -1] = 0

        # Add return with spread
        spread = 15
        start = distance - spread
        end = distance + spread
        for i in range(start, end):
            intensity = max(0, 1 - abs(i - distance) / spread)  # Linear falloff
            grid[i, -1] = max(grid[i, -1], intensity)

        img.set_data(grid)
        fig.canvas.draw()
        fig.canvas.flush_events()