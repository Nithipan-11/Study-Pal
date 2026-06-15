#include <Wire.h>
#include <U8g2lib.h>

#define GREEN_LED  8
#define RED_LED    9
#define BUZZER     10

// _2_ page buffer = 256 bytes RAM, stable I2C for SH1106

U8G2_SH1106_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

char    inputBuffer[32];
uint8_t inputLen = 0;

// ── Active buzzer beep state ──────────────────────────────────
// Active buzzer = 2 pins, just needs HIGH/LOW. tone() is wrong for this.
int           beepsLeft      = 0;
bool          continuousBeep = false;  // true = repeat forever (AWAY, PHONE)
bool          beepPhaseOn    = false;  // true = buzzer currently HIGH
unsigned long beepPhaseTime  = 0;
#define BEEP_ON_MS  200
#define BEEP_OFF_MS 200

// ── AWAY blink sequence ───────────────────────────────────────
bool          awayBlinking = false;
int           blinkHalf    = 0;        // 0-5 = 3 full blinks
unsigned long blinkTime    = 0;

// ── BREAK: turn green off after last beep ────────────────────
bool greenOffAfterBeep = false;

// ─────────────────────────────────────────────────────────────
void allOff() {
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED,   LOW);
  digitalWrite(BUZZER, LOW);  // active buzzer: LOW = silent

  beepsLeft         = 0;
  continuousBeep    = false;
  beepPhaseOn       = false;
  awayBlinking      = false;
  greenOffAfterBeep = false;
}

// ─────────────────────────────────────────────────────────────
// Counted beeps (BREAK 2x) — stops after count
void startBeeps(int count) {
  beepsLeft      = count;
  continuousBeep = false;
  beepPhaseOn    = true;
  beepPhaseTime  = millis();
  digitalWrite(BUZZER, HIGH);
}

// Repeating beeps forever (AWAY, PHONE) — stops only when allOff() is called
void startContinuousBeep() {
  continuousBeep = true;
  beepsLeft      = 0;
  beepPhaseOn    = true;
  beepPhaseTime  = millis();
  digitalWrite(BUZZER, HIGH);
}

void startAwayBlink() {
  awayBlinking = true;
  blinkHalf    = 0;
  blinkTime    = millis() - 200; // fire first toggle immediately
}

// ─────────────────────────────────────────────────────────────
void updateBlink() {
  if (!awayBlinking) return;
  if (millis() - blinkTime < 200) return;

  blinkTime = millis();
  digitalWrite(RED_LED, (blinkHalf % 2 == 0) ? HIGH : LOW);
  blinkHalf++;

  if (blinkHalf >= 6) {
    awayBlinking = false;
    digitalWrite(RED_LED, HIGH);  // latch red on
    startContinuousBeep();        // beep repeatedly until allOff()
  }
}

void updateBeep() {
  if (!continuousBeep && beepsLeft <= 0) return;

  if (beepPhaseOn) {
    // Buzzer ON phase — turn off after BEEP_ON_MS
    if (millis() - beepPhaseTime >= BEEP_ON_MS) {
      digitalWrite(BUZZER, LOW);
      beepPhaseOn   = false;
      beepPhaseTime = millis();
      if (!continuousBeep) {
        beepsLeft--;
        if (beepsLeft == 0 && greenOffAfterBeep) {
          digitalWrite(GREEN_LED, LOW);
          greenOffAfterBeep = false;
        }
      }
    }
  } else {
    // Gap phase — start next beep after BEEP_OFF_MS
    if (millis() - beepPhaseTime >= BEEP_OFF_MS) {
      if (continuousBeep || beepsLeft > 0) {
        digitalWrite(BUZZER, HIGH);
        beepPhaseOn   = true;
        beepPhaseTime = millis();
      }
    }
  }
}

// ─────────────────────────────────────────────────────────────
// showMessage always called BEFORE activating LEDs/buzzer
// so the I2C update isn't disrupted by power draw
void showMessage(const char* line1, const char* line2) {
  u8g2.firstPage();
  do {
    if (line2 && line2[0] != '\0') {
      u8g2.setFont(u8g2_font_helvB12_tr);
      u8g2.setCursor(0, 20);
      u8g2.print(line1);
      u8g2.setFont(u8g2_font_helvB10_tr);
      u8g2.setCursor(0, 44);
      u8g2.print(line2);
    } else {
      u8g2.setFont(u8g2_font_helvB14_tr);
      u8g2.setCursor(0, 38);
      u8g2.print(line1);
    }
  } while (u8g2.nextPage());
}

// ─────────────────────────────────────────────────────────────
void handleCommand(const char* cmd) {
  Serial.print("[RX] ");
  Serial.println(cmd);

  if (strcmp(cmd, "FOCUS") == 0) {
    allOff();
    showMessage("Focus Mode", "");
    digitalWrite(GREEN_LED, HIGH);
    Serial.println("[FOCUS] Green ON | Red OFF | Buzzer OFF");
  }
  else if (strcmp(cmd, "PHONE") == 0) {
    allOff();
    showMessage("Phone Detected", "Put it away!");
    digitalWrite(RED_LED, HIGH);
    startContinuousBeep();
    Serial.println("[PHONE] Red ON | Buzzer continuous");
  }
  else if (strcmp(cmd, "AWAY") == 0) {
    allOff();
    showMessage("Come Back!", "");
    // Green OFF — allOff() handled it
    startAwayBlink();  // red blinks x3 → latches ON → buzzer ON continuous
    Serial.println("[AWAY] Red blink x3 -> ON | Buzzer ON");
  }
  else if (strcmp(cmd, "BREAK") == 0) {
    allOff();
    showMessage("Break Time!", "");
    // Red OFF — allOff() handled it
    digitalWrite(GREEN_LED, HIGH);
    greenOffAfterBeep = true;
    startBeeps(2);
    Serial.println("[BREAK] Green ON->OFF after 2 beeps | Red OFF");
  }
  else {
    Serial.print("[UNKNOWN] ");
    Serial.println(cmd);
  }
}

// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED,   OUTPUT);
  pinMode(BUZZER,    OUTPUT);
  allOff();
  u8g2.begin();
  showMessage("Study Pal", "Ready");
  Serial.println("[BOOT] Study Pal ready.");
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputLen > 0) {
        inputBuffer[inputLen] = '\0';
        handleCommand(inputBuffer);
        inputLen = 0;
      }
    } else if (inputLen < 31) {
      inputBuffer[inputLen++] = c;
    }
  }

  updateBlink();
  updateBeep();
}
