/* =========================================================
   Radar Ultrasónico + Servo + LED + Buzzer + Botón
   *** ADAPTADO PARA PANTALLA OLED SSD1306 128x64 ***
   ========================================================= */

#include <Servo.h>

// --- LIBRERÍAS OLED ---
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- CONFIG BUZZER ---
#define BUZZER_PASSIVE 0

// --- CONFIG OLED ---
#define SCREEN_WIDTH 128 // Ancho en pixeles
#define SCREEN_HEIGHT 64 // Alto en pixeles
#define OLED_RESET    -1 // Pin de Reset (-1 si comparte pin RST de Arduino)
// Inicializa la pantalla (I2C)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --------- Pines --------------
const int PIN_TRIG   = 8;
const int PIN_ECHO   = 9;
const int PIN_SERVO  = 6;
const int PIN_LED    = 12;
const int PIN_BUZZER = 11;
const int PIN_BUTTON = 2;  // a GND (INPUT_PULLUP)

// --------- Escaneo ------------
const int  ANG_MIN = 15;
const int  ANG_MAX = 165;
const int  STEP_ANGLE = 3;
const unsigned long STEP_DELAY_MS = 100; // más lento/suave

// --------- Umbrales -----------
const float THRESHOLD_ON_CM  = 20.0; // dispara
const float THRESHOLD_OFF_CM = 24.0; // apaga (histéresis)

// --------- Alarmas ------------
const unsigned long BUZZ_ON_MS   = 200;
const unsigned long BUZZ_OFF_MS  = 200;
const unsigned long LED_BLINK_MS = 300; // Blink LED y OLED

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
int   bestAngle    = ANG_MIN;

// Buzzer/LED/OLED no-bloqueante
bool buzzOn = false;
unsigned long lastBuzzMillis = 0;
bool ledOn = false;
unsigned long lastLedMillis = 0;

// Botón
bool lastBtnState = HIGH;

// Movimiento suave en ALARM
int smoothTarget = ANG_MIN;
int smoothPos    = ANG_MIN;
unsigned long lastSmoothStepMillis = 0;

// --- Salida estable de ALARM ---
int safeCount = 0;
const int SAFE_REQUIRED = 3;

// =========================================================
void setup() {
  Serial.begin(115200);

  // --- INICIAR OLED ---
  // Dirección 0x3C es la más común para OLEDs 128x64
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Fallo al iniciar SSD1306"));
    for(;;); // No continuar si falla
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println(F("Radar HC-SR04"));
  display.setCursor(0, 10);
  display.println(F("Iniciando..."));
  display.display(); // Mostrar en pantalla
  // --- FIN OLED ---

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
  delay(1000); // Dar tiempo a leer el mensaje OLED

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
      delay(10); // asentamiento

      float d = measureDistanceCM();

      Serial.print("Ángulo: "); Serial.print(currentAngle);
      Serial.print("°  Distancia: "); Serial.print(d); Serial.println(" cm");

      // --- OLED: Dibujar pantalla de escaneo ---
      display.clearDisplay();
      
      // Fila 1: Estado y Ángulo
      display.setTextSize(2); // Texto grande
      display.setCursor(0, 0);
      display.print(F("SCAN: "));
      display.print(currentAngle);
      display.print((char)247); // Símbolo de Grado
      
      // Fila 2: Distancia
      display.setTextSize(2); // Texto grande
      display.setCursor(0, 25);
      if (d < 0) {
        display.print(F("Dist: ---"));
      } else {
        display.print(F("Dist: "));
        display.print(d, 1); // Distancia con 1 decimal
      }
      
      // Fila 3: "cm" (más pequeño)
      display.setTextSize(1);
      display.setCursor(0, 45);
      if (d >= 0) {
         display.print(F("cm"));
      }

      // Barra de progreso del ángulo en la parte inferior
      int barWidth = map(currentAngle, ANG_MIN, ANG_MAX, 0, SCREEN_WIDTH - 2);
      display.drawRect(0, SCREEN_HEIGHT - 3, SCREEN_WIDTH, 3, SSD1306_WHITE);
      display.fillRect(1, SCREEN_HEIGHT - 2, barWidth, 1, SSD1306_WHITE);

      display.display(); // Actualizar pantalla
      // --- FIN OLED ---

      // Disparo inmediato
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
    // Mantenerse mirando al objeto
    smoothTarget = bestAngle;
    smoothAimStep();

    handleAlarmOutputs(); // Esto ahora también controla el parpadeo de la OLED

    // Re-medición con salida robusta
    float d = measureDistanceCM();
    if (d > 0 && d <= THRESHOLD_ON_CM) {
      safeCount = 0;
    } else if (d < 0 || d > THRESHOLD_OFF_CM) {
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
      // zona muerta, no hacer nada
    }
  }
}

// =========================================================
void enterAlarmImmediate() {
  state = ALARM;
  lastBuzzMillis = millis();
  lastLedMillis  = millis();
  buzzOn = false; ledOn = false;
  safeCount = 0;

  // preparar apuntado suave
  smoothPos = myServo.read();
  smoothTarget = bestAngle;
  lastSmoothStepMillis = millis();

  // --- OLED: Dibujar pantalla de ALARMA ---
  display.clearDisplay();
  
  // Título de Alarma
  display.setTextSize(2);
  display.setCursor(15, 8); // Centrado aprox
  display.print(F("! ALARMA !"));
  
  // Datos del objeto
  display.setTextSize(1);
  display.setCursor(0, 30);
  display.print(F("Objeto detectado a:"));
  
  display.setTextSize(2); // Grande
  display.setCursor(0, 40);
  display.print(bestDistance, 1);
  display.print(F(" cm"));
  
  display.setTextSize(1);
  display.setCursor(80, 50); // Esquina
  display.print(F("@ "));
  display.print(bestAngle);
  display.print((char)247); // Grado
  
  display.display();
  // --- FIN OLED ---

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
      tone(PIN_BUZZER, 3000);
#else
      digitalWrite(PIN_BUZZER, HIGH);
#endif
    }
  }

  // LED blink Y OLED BLINK (INVERT)
  if (now - lastLedMillis >= LED_BLINK_MS) {
    lastLedMillis = now;
    ledOn = !ledOn;
    digitalWrite(PIN_LED, ledOn ? HIGH : LOW);
    
    // --- OLED ---
    // Hacer "parpadear" la pantalla invirtiendo colores
    display.invertDisplay(ledOn);
    // --- FIN OLED ---
  }
}

// =========================================================
float measureDistanceCM() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long duration = pulseIn(PIN_ECHO, HIGH, 30000UL); // 30 ms
  if (duration == 0) return -1.0;

  return duration / 58.2; // cm
}

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

  // --- OLED ---
  display.invertDisplay(false); // Asegurarse que la pantalla NO esté invertida
  display.clearDisplay(); 
  display.setTextSize(1);
  display.setCursor(0, 10); 
  display.println(F("Alarma detenida."));
  display.setCursor(0, 20); 
  display.println(F("Reanudando escaneo...")); 
  display.display(); 
  delay(1000); // Dar tiempo a leer
  // --- FIN OLED ---
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
