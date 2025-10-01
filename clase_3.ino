/* =========================================================
   Radar Ultrasónico (HC-SR04 4 pines) + Servo + LED + Buzzer + Botón
   Pines:
     HC-SR04: Trig→8, Echo→9 | Servo→6 | Buzzer→11 | LED→12 | Botón→2 (a GND)
   Función:
     - Barre 15↔165° en pasos de 3°
     - Si detecta ≤ 20 cm: ENTRA EN ALARMA INMEDIATO,
       APUNTA AL OBJETO y SE QUEDA allí (no sigue escaneando)
     - Alarma (LED + buzzer intermitente)
     - Sale de ALARMA sólo si hay 3 lecturas seguidas >24 cm (histéresis robusta)
       o si se presiona el botón
     - Serial: ángulo, distancia, estado, mejor objeto, alarmas, botón
   ========================================================= */

#include <Servo.h>

// ------- CONFIG BUZZER -------
// 0 = buzzer ACTIVO (HIGH/LOW)   |  1 = buzzer PASIVO (tone/noTone)
#define BUZZER_PASSIVE 0

// --------- Pines --------------
const int PIN_TRIG   = 8;
const int PIN_ECHO   = 9;
const int PIN_SERVO  = 6;
const int PIN_LED    = 12;
const int PIN_BUZZER = 11;
const int PIN_BUTTON = 2;  // a GND (INPUT_PULLUP)

// --------- Escaneo ------------
const int  ANG_MIN = 15;
const int  ANG_MAX = 165;
const int  STEP_ANGLE = 3;
const unsigned long STEP_DELAY_MS = 100; // más lento/suave

// --------- Umbrales -----------
const float THRESHOLD_ON_CM  = 20.0; // dispara
const float THRESHOLD_OFF_CM = 24.0; // apaga (histéresis)

// --------- Alarmas ------------
const unsigned long BUZZ_ON_MS   = 200;
const unsigned long BUZZ_OFF_MS  = 200;
const unsigned long LED_BLINK_MS = 300;

// --------- Suavizado apuntado en ALARM ----
const unsigned long SMOOTH_STEP_MS = 15; // 1° cada 15 ms

// --------- Objetos/estado -----
Servo myServo;
enum State { SCANNING, ALARM };
State state = SCANNING;

int currentAngle = ANG_MIN;
int stepSign = 1;
unsigned long lastStepMillis = 0;

float bestDistance = 9999.0;
int   bestAngle    = ANG_MIN;

// Buzzer/LED no-bloqueante
bool buzzOn = false;
unsigned long lastBuzzMillis = 0;
bool ledOn = false;
unsigned long lastLedMillis = 0;

// Botón
bool lastBtnState = HIGH;

// Movimiento suave en ALARM
int smoothTarget = ANG_MIN;
int smoothPos    = ANG_MIN;
unsigned long lastSmoothStepMillis = 0;

// --- Salida estable de ALARM ---
int safeCount = 0;                // lecturas consecutivas "alejado"
const int SAFE_REQUIRED = 3;      // cuántas seguidas para apagar

// =========================================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

#if BUZZER_PASSIVE
  noTone(PIN_BUZZER);
#else
  digitalWrite(PIN_BUZZER, LOW);
#endif
  digitalWrite(PIN_LED, LOW);

  myServo.attach(PIN_SERVO);
  myServo.write(currentAngle);
  smoothPos = currentAngle;
  delay(200);

  Serial.println("Estado: SCANNING");
}

// =========================================================
void loop() {
  handleButton();

  if (state == SCANNING) {
    unsigned long now = millis();
    if (now - lastStepMillis >= STEP_DELAY_MS) {
      lastStepMillis = now;

      myServo.write(currentAngle);
      delay(10); // breve asentamiento mecánico

      float d = measureDistanceCM();

      Serial.print("Ángulo: "); Serial.print(currentAngle);
      Serial.print("°  Distancia: "); Serial.print(d); Serial.println(" cm");

      // Disparo inmediato: si ve algo ≤20 cm, entra a ALARM ya
      if (d > 0 && d <= THRESHOLD_ON_CM) {
        bestDistance = d;
        bestAngle = currentAngle;
        enterAlarmImmediate();
        return;
      }

      // (opcional) mejor de la pasada
      if (d > 0 && d < bestDistance) {
        bestDistance = d;
        bestAngle = currentAngle;
      }

      // Avance de barrido
      currentAngle += stepSign * STEP_ANGLE;
      if (currentAngle >= ANG_MAX) {
        currentAngle = ANG_MAX; stepSign = -1;
        reportBestAfterPass();
        bestDistance = 9999.0; bestAngle = (stepSign > 0) ? ANG_MIN : ANG_MAX;
      } else if (currentAngle <= ANG_MIN) {
        currentAngle = ANG_MIN; stepSign = 1;
        reportBestAfterPass();
        bestDistance = 9999.0; bestAngle = (stepSign > 0) ? ANG_MIN : ANG_MAX;
      }
    }

  } else if (state == ALARM) {
    // Mantenerse mirando al objeto, sin escanear
    smoothTarget = bestAngle;
    smoothAimStep();

    handleAlarmOutputs();

    // Re-medición con salida robusta (requiere SAFE_REQUIRED lecturas "alejado")
    float d = measureDistanceCM();
    if (d > 0 && d <= THRESHOLD_ON_CM) {
      // sigue cerca -> mantener alarma y resetear contador de salida
      safeCount = 0;
    } else if (d < 0 || d > THRESHOLD_OFF_CM) {
      // posible salida -> contar
      safeCount++;
      if (safeCount >= SAFE_REQUIRED) {
        stopAlarm();
        Serial.println(">>> ALARMA DESACTIVADA <<<");
        Serial.println("Estado: SCANNING");
        resetScan();
        state = SCANNING;
        safeCount = 0;
      }
    } else {
      // zona muerta entre ON y OFF: no cambiar estado, pero no sumar
      // (mantiene estabilidad de la histéresis)
    }
  }
}

// =========================================================
void enterAlarmImmediate() {
  state = ALARM;
  lastBuzzMillis = millis();
  lastLedMillis  = millis();
  buzzOn = false; ledOn = false;
  safeCount = 0; // reset contador de salida estable

  // preparar apuntado suave
  smoothPos = myServo.read();
  smoothTarget = bestAngle;
  lastSmoothStepMillis = millis();

  Serial.print("Más cercano (inmediato): ");
  Serial.print(bestDistance); Serial.print(" cm @ ");
  Serial.print(bestAngle); Serial.println("°");
  Serial.println(">>> ALARMA ACTIVADA <<<");
  Serial.println("Estado: ALARM");
}

// =========================================================
void reportBestAfterPass() {
  Serial.print("Más cercano (pasada): ");
  Serial.print(bestDistance); Serial.print(" cm @ ");
  Serial.print(bestAngle); Serial.println("°");
}

// =========================================================
//           Movimiento SUAVE hacia smoothTarget
// =========================================================
void smoothAimStep() {
  unsigned long now = millis();
  if (now - lastSmoothStepMillis < SMOOTH_STEP_MS) return;
  lastSmoothStepMillis = now;

  if (smoothPos < smoothTarget) {
    smoothPos++;
    myServo.write(smoothPos);
  } else if (smoothPos > smoothTarget) {
    smoothPos--;
    myServo.write(smoothPos);
  }
}

// =========================================================
//                  Buzzer + LED intermitentes
// =========================================================
void handleAlarmOutputs() {
  unsigned long now = millis();

  // Buzzer intermitente
  if (buzzOn) {
    if (now - lastBuzzMillis >= BUZZ_ON_MS) {
      lastBuzzMillis = now;
      buzzOn = false;
#if BUZZER_PASSIVE
      noTone(PIN_BUZZER);
#else
      digitalWrite(PIN_BUZZER, LOW);
#endif
    }
  } else {
    if (now - lastBuzzMillis >= BUZZ_OFF_MS) {
      lastBuzzMillis = now;
      buzzOn = true;
#if BUZZER_PASSIVE
      tone(PIN_BUZZER, 3000); // 3 kHz aprox
#else
      digitalWrite(PIN_BUZZER, HIGH);
#endif
    }
  }

  // LED blink
  if (now - lastLedMillis >= LED_BLINK_MS) {
    lastLedMillis = now;
    ledOn = !ledOn;
    digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
  }
}

// =========================================================
//                Medición con HC-SR04 (4 pines)
// =========================================================
float measureDistanceCM() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long duration = pulseIn(PIN_ECHO, HIGH, 30000UL); // 30 ms
  if (duration == 0) return -1.0;

  return duration / 58.2; // cm
}

// =========================================================
//                      Botón (silencio)
// =========================================================
void handleButton() {
  bool reading = digitalRead(PIN_BUTTON);
  if (lastBtnState == HIGH && reading == LOW) {
    Serial.println("Botón presionado: alarma silenciada");
    if (state == ALARM) {
      stopAlarm();
      Serial.println(">>> ALARMA DESACTIVADA <<<");
      Serial.println("Estado: SCANNING");
      resetScan();
      state = SCANNING;
      safeCount = 0;
    } else {
      resetScan();
    }
  }
  lastBtnState = reading;
}

// =========================================================
void stopAlarm() {
#if BUZZER_PASSIVE
  noTone(PIN_BUZZER);
#else
  digitalWrite(PIN_BUZZER, LOW);
#endif
  digitalWrite(PIN_LED, LOW);
  buzzOn = false; ledOn = false;
}

// =========================================================
void resetScan() {
  bestDistance = 9999.0;
  bestAngle = ANG_MIN;
  currentAngle = ANG_MIN;
  stepSign = 1;
  lastStepMillis = millis();
  // suave
  smoothPos = currentAngle;
  smoothTarget = currentAngle;
  lastSmoothStepMillis = millis();
}
