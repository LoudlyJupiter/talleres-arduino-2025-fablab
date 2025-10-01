// ====== Pines ======
const int PIN_NTC  = A0;   // Nodo del divisor (NTC ↔ R1)
const int LED      = 9;   // LED indicador
const int BTN_UP   = 2;   // Botón subir centro
const int BTN_DOWN = 3;   // Botón bajar centro

// ====== Constantes eléctricas / ADC ======
const float VREF = 5.0;       // Ref ADC (Arduino UNO típico)
const float R1   = 10000.0;   // Resistencia fija (10k)

// ====== Steinhart–Hart (ajusta según tu NTC) ======
const float A = 1.129148e-3;
const float B = 2.341250e-4;
const float C = 8.767410e-8;

// ====== Zona objetivo ======
int center = 30;   // °C (ajustable con botones)
int window = 2;    // ±°C

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(BTN_UP,   INPUT_PULLUP);   // Botones a GND (LOW = presionado)
  pinMode(BTN_DOWN, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println(F("Juego 'Mantén la Zona' iniciado"));
  printZona();
}

void loop() {
  // --- 1) Leer ADC y convertir a voltaje ---
  int adc = analogRead(PIN_NTC);
  float Vout = (adc / 1023.0) * VREF;

  // Evitar división por cero
  if (Vout < 0.005) Vout = 0.005;

  // --- 2) Vout -> Rntc (NTC ARRIBA, R1 ABAJO) ---
  float Rntc = R1 * (VREF - Vout) / Vout;

  // --- 3) Steinhart–Hart -> Kelvin -> °C ---
  float lnR = log(Rntc);
  float invT = A + B*lnR + C*pow(lnR, 3);
  float tempK = 1.0 / invT;
  float tempC = tempK - 273.15;

  // --- 4) LED según zona (no bloqueante con millis) ---
  unsigned long now = millis();
  bool inRange = (tempC >= center - window && tempC <= center + window);

  if (inRange) {
    digitalWrite(LED, HIGH);                // dentro: fijo
  } else if (tempC < center - window) {
    bool blink = ((now / 500) % 2) == 0;    // frío: parpadeo lento (1 Hz)
    digitalWrite(LED, blink ? HIGH : LOW);
  } else {
    bool blink = ((now / 100) % 2) == 0;    // caliente: parpadeo rápido (5 Hz)
    digitalWrite(LED, blink ? HIGH : LOW);
  }

  // --- 5) Log por Serial cada ~500 ms ---
  static unsigned long tLog = 0;
  if (now - tLog > 500) {
    tLog = now;
    Serial.print(F("ADC="));   Serial.print(adc);
    Serial.print(F(" | Vout=")); Serial.print(Vout, 3); Serial.print(F(" V"));
    Serial.print(F(" | Rntc=")); Serial.print(Rntc, 0); Serial.print(F(" ohm"));
    Serial.print(F(" | Temp=")); Serial.print(tempC, 1); Serial.print(F(" °C"));
    Serial.print(F(" | Zona: [")); Serial.print(center - window);
    Serial.print(F(","));       Serial.print(center + window);
    Serial.println(F("]"));
  }

  // --- 6) Botones (antirrebote simple con delay) ---
  if (digitalRead(BTN_UP) == LOW) {
    center += 1;
    printZona();
    delay(200); // antirrebote básico
  }
  if (digitalRead(BTN_DOWN) == LOW) {
    center -= 1;
    printZona();
    delay(200);
  }
}

void printZona() {
  Serial.print(F("Nueva zona objetivo: "));
  Serial.print(center - window); Serial.print(F(" a "));
  Serial.print(center + window); Serial.println(F(" °C"));
}
