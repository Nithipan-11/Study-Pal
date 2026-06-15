# Study Pal

A webcam-based study monitor that uses computer vision to track focus and alert you when you're distracted. Sends real-time mode signals to an Arduino over serial.

## How it works

- **Face detection** (OpenCV Haar cascade) — determines whether you're at your desk
- **Phone detection** (YOLOv8) — flags phone use immediately
- **Mode signals** sent to Arduino: `FOCUS`, `AWAY`, `BREAK`, `PHONE`

## Modes

| Mode | Trigger |
|------|---------|
| FOCUS | Face detected, no phone |
| AWAY | No face detected |
| BREAK | Manually toggled with `b` key |
| PHONE | Phone detected (overrides all) |

## Requirements

- Python 3.8+
- Arduino connected on `COM3` at 9600 baud
- Webcam

Install dependencies:

```
pip install opencv-python ultralytics pyserial
```

## Usage

```
python study_pal.py
```

- `b` — toggle break mode
- `q` — quit
