import cv2
import serial
import time
from ultralytics import YOLO

SERIAL_PORT          = 'COM3'
BAUD_RATE            = 9600
PHONE_CLASS_ID       = 67
CONFIDENCE_THRESHOLD = 0.5
MODE_MIN_HOLD        = 1.5   # seconds — lets Arduino finish blink/beep before switching

# Face detection: OpenCV Haar cascade
face_cascade = cv2.CascadeClassifier(
    cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
)

# Phone detection: YOLOv8
model = YOLO('yolov8n.pt')

try:
    arduino = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    time.sleep(2)
    print(f"Connected to Arduino on {SERIAL_PORT}")
except serial.SerialException as e:
    print(f"Could not open {SERIAL_PORT}: {e}")
    raise SystemExit(1)


def send_command(command):
    arduino.write((command + '\n').encode())
    arduino.flush()


cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("Could not open webcam.")
    arduino.close()
    raise SystemExit(1)

current_mode     = None
break_mode       = False
last_switch_time = 0.0

print("Study Pal running.  b = toggle break mode   q = quit")

try:
    while True:
        ret, frame = cap.read()
        if not ret:
            break

        key = cv2.waitKey(1) & 0xFF
        if key == ord('q'):
            break
        elif key == ord('b'):
            break_mode = not break_mode
            if not break_mode:
                current_mode = None  # force re-evaluation on exit

        # ── Face detection (OpenCV) ──────────────────────────
        gray  = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        faces = face_cascade.detectMultiScale(
            gray, scaleFactor=1.1, minNeighbors=5, minSize=(80, 80)
        )
        face_detected = len(faces) > 0
        for (x, y, w, h) in faces:
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)

        # ── Phone detection (YOLOv8) ─────────────────────────
        results       = model(frame, verbose=False)
        phone_detected = False
        for result in results:
            for box in result.boxes:
                if (int(box.cls[0]) == PHONE_CLASS_ID
                        and float(box.conf[0]) > CONFIDENCE_THRESHOLD):
                    phone_detected = True
                    x1, y1, x2, y2 = map(int, box.xyxy[0])
                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 0, 255), 2)
                    cv2.putText(frame, f'Phone {box.conf[0]:.0%}', (x1, y1 - 8),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

        # ── Mode priority: PHONE > BREAK > FOCUS > AWAY ──────
        if phone_detected:
            new_mode = 'PHONE'
        elif break_mode:
            new_mode = 'BREAK'
        elif face_detected:
            new_mode = 'FOCUS'
        else:
            new_mode = 'AWAY'

        if new_mode != current_mode:
            now = time.time()
            # PHONE always overrides instantly; others respect the hold timer
            if new_mode == 'PHONE' or (now - last_switch_time) >= MODE_MIN_HOLD:
                current_mode     = new_mode
                send_command(current_mode)
                last_switch_time = now
                print(f"Mode -> {current_mode}")

        # ── HUD overlay ──────────────────────────────────────
        label_color = (0, 255, 0) if current_mode in ('FOCUS', 'BREAK') else (0, 0, 255)
        cv2.putText(frame, f'Mode: {current_mode}', (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 1.0, label_color, 2)
        cv2.putText(frame,
                    '[BREAK ON]' if break_mode else 'b=break  q=quit',
                    (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (200, 200, 200), 1)
        cv2.imshow('Study Pal', frame)

except KeyboardInterrupt:
    pass
finally:
    cap.release()
    cv2.destroyAllWindows()
    arduino.close()
    print("Study Pal stopped.")
