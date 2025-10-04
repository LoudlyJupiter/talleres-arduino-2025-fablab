// ====== Pines ======
const int PIN_NTC  = A0;    // Nodo del divisor (NTC ↔ R1)
const int LED      = 9;     // LED indicador

// ¡IMPORTANTE! Reutilizamos estos pines para los botones del juego.
const int BTN_UP   = 2;     // Botón subir centro (ahora Botón de JUEGO A)
const int BTN_DOWN = 3;     // Botón bajar centro (ahora Botón de JUEGO B)

// ====== Constantes eléctricas / ADC ======
const float VREF = 5.0;     // Ref ADC (Arduino UNO típico)
const float R1   = 10000.0; // Resistencia fija (10k)

// ====== Steinhart–Hart (ajusta según tu NTC) ======
const float A = 1.129148e-3;
const float B = 2.341250e-4;
const float C = 8.767410e-8;

// ====== Zona objetivo ======
int center = 30;    // °C (ajustable con botones, si los mantienes)
int window = 2;     // ±°C

// ====== VARIABLES DEL TEMPORIZADOR DEL JUEGO ======
unsigned long tiempoInicio = 0;  // Momento en que el jugador entró a la zona
bool jugadorEnZona = false;      // Estado actual del juego
unsigned long tiempoMaximo = 0;   // El "High Score" (máxima duración)

// Variables para el parpadeo
unsigned long tLog = 0;

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(BTN_UP,   INPUT_PULLUP);    // Botones a GND (LOW = presionado)
  pinMode(BTN_DOWN, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println(F("--- JUEGO 'MANTÉN LA ZONA' INICIADO ---"));
  printZona();
}

void loop() {
  unsigned long now = millis();

  // --- 1) Leer ADC y convertir a Temp °C (el código original es correcto) ---
  int adc = analogRead(PIN_NTC);
  float Vout = (adc / 1023.0) * VREF;
  if (Vout < 0.005) Vout = 0.005; // Evitar división por cero

  float Rntc = R1 * (VREF - Vout) / Vout;
  float lnR = log(Rntc);
  float invT = A + B*lnR + C*pow(lnR, 3);
  float tempK = 1.0 / invT;
  float tempC = tempK - 273.15;

  // --- 2) CONDICIÓN DE ZONA ---
  bool enRangoTemp = (tempC >= center - window && tempC <= center + window);
  
  // Condición de la "Zona" para el juego: Estar en el rango de temperatura
  // Si deseas incluir los botones como requisito (como en el ejemplo anterior), usa:
  // bool condicionZona = enRangoTemp && (digitalRead(BTN_UP) == LOW) && (digitalRead(BTN_DOWN) == LOW);
  // Por simplicidad, usaremos solo la temperatura:
  bool condicionZona = enRangoTemp; 

  // --- 3) LÓGICA DEL TEMPORIZADOR DEL JUEGO ---
  if (condicionZona && !jugadorEnZona) {
    // INICIO DEL TIEMPO: Acaba de entrar en la zona
    jugadorEnZona = true;
    tiempoInicio = now;
    Serial.println(F("\n-> ¡JUGADOR EN LA ZONA! Tiempo iniciado."));

  } else if (!condicionZona && jugadorEnZona) {
    // FIN DEL TIEMPO: Acaba de salir de la zona
    
    unsigned long duracion = now - tiempoInicio;
    jugadorEnZona = false;
    
    // Actualizar el tiempo máximo (High Score)
    if (duracion > tiempoMaximo) {
      tiempoMaximo = duracion;
    }

    // Reporte Final
    Serial.print(F("<- ¡ZONA PERDIDA! Duración: "));
    Serial.print(duracion / 1000.0); Serial.println(F(" s"));
    Serial.print(F("¡TIEMPO MÁXIMO (HS) REGISTRADO: "));
    Serial.print(tiempoMaximo / 1000.0); Serial.println(F(" s!"));
    Serial.println(F("----------------------------------------"));
  }

  // --- 4) Feedback Visual (LED) y Log Serial ---
  // Reutiliza la lógica no bloqueante del código original
  
  // LED según zona
  if (condicionZona) {
    digitalWrite(LED, HIGH); // dentro: fijo (y contando)
  } else if (tempC < center - window) {
    bool blink = ((now / 500) % 2) == 0; // frío: parpadeo lento (1 Hz)
    digitalWrite(LED, blink ? HIGH : LOW);
  } else {
    bool blink = ((now / 100) % 2) == 0; // caliente: parpadeo rápido (5 Hz)
    digitalWrite(LED, blink ? HIGH : LOW);
  }

  // Log por Serial
  if (now - tLog > 500) {
    tLog = now;
    Serial.print(F("Temp=")); Serial.print(tempC, 1); Serial.print(F(" °C"));
    Serial.print(F(" | Zona: [")); Serial.print(center - window);
    Serial.print(F(" a ")); Serial.print(center + window); Serial.print(F("]"));
    
    if (jugadorEnZona) {
        // Mostrar el tiempo actual de la partida
        unsigned long tiempoActual = (now - tiempoInicio) / 1000;
        Serial.print(F(" | CONTEO: ")); Serial.print(tiempoActual); Serial.print(F(" s"));
    }
    Serial.println();
  }

  // --- 5) Botones (Antirrebote simple con delay. Puedes usar millis() si es necesario) ---
  // Mantenemos la funcionalidad de los botones originales para ajustar la dificultad
  if (digitalRead(BTN_UP) == LOW) {
    center += 1;
    printZona();
    delay(200);
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
