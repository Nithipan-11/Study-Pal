# Study Pal

An Arduino + webcam project that detects your focus level and responds with hardware feedback.

## Hardware

| Component | Arduino Pin | Notes |
|---|---|---|
| Green LED | D8 | via 220Ω resistor — focused indicator |
| Red LED | D9 | via 220Ω resistor — distracted indicator |
| Buzzer | D10 | audio alerts |
| OLED display | A4 (SDA), A5 (SCK) | 1.3" SH1106, I2C address 0x3C, U8g2 library |

**Other parts:** Arduino Uno, breadboard, jumper wires, 220Ω resistors (x2 for LEDs).

**Wiring notes:**
- Common ground — all GND legs/pins connect to the same blue rail, which connects to Arduino GND
- LED long leg = positive (anode) → toward resistor/Arduino pin; short leg = negative (cathode) → GND
- Transistor (flat side facing you): left = Emitter → GND, middle = Base → 1kΩ → D6, right = Collector → motor
- OLED only needs 4 wires (VCC, GND, SDA, SCK) — no potentiometer required

## Software

- **StudyPal.ino** — Arduino sketch. Reads serial commands (FOCUS, PHONE, AWAY, BREAK) and controls LEDs, buzzer and OLED.
- **study_pal.py** — Python script using OpenCV for face/head position detection and YOLOv8 for phone detection. Sends serial commands to Arduino.

## Setup

### Arduino
1. Install the **U8g2** library via Library Manager
2. Upload `StudyPal.ino`

### Python
```
pip install opencv-python pyserial ultralytics
```

Edit `SERIAL_PORT` in `study_pal.py` to match your Arduino's COM port.

### Run
```
python study_pal.py
```

Press **B** for break mode, **Q** to quit.

## Modes

| Mode | Green LED | Red LED | Buzzer | OLED |
|---|---|---|---|---|---|
| FOCUS | ON | OFF | OFF | OFF | "Focus Mode" |
| PHONE | ON | ON | ON | 3 beeps | "Phone Detected" |
| AWAY | OFF | ON (blinks then stays) | ON | Continuous | "Come Back!" |
| BREAK | ON | OFF | OFF | 2 beeps | "Break Time!" |

## How it works

OpenCV's Haar cascade detects your face each frame. If your face is centered in the webcam view, the system counts you as focused (FOCUS). If your face moves off-center or disappears for a few seconds, it switches to AWAY. YOLOv8 runs in parallel to detect a phone in frame — if found, PHONE mode triggers regardless of face position. Pressing B manually triggers BREAK, which cancels automatically once your face returns to center.
