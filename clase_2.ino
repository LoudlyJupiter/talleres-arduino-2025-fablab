// -------------------------------------------------------------------
// --- Pines y Constantes de Hardware ---
// -------------------------------------------------------------------
const int PIN_NTC      = A0;    // Nodo del divisor (NTC ↔ R1)
const int LED          = 9;     // LED indicador de estado y zona
const int BTN_UP       = 2;     // Botón para subir la temperatura objetivo
const int BTN_DOWN     = 3;     // Botón para bajar la temperatura objetivo

// Constantes eléctricas / ADC
const float VREF       = 5.0;     // Voltaje de referencia del ADC (Arduino UNO típico)
const float R1         = 10000.0; // Resistencia fija del divisor (10k Ohms)

// Steinhart–Hart (A, B, C de tu NTC)
const float A = 1.129148e-3;
const float B = 2.341250e-4;
const float C = 8.767410e-8;

// Zona objetivo (Ajustable con botones)
int center = 30;    // °C (Centro de la zona de juego)
int window = 2;     // ±°C (Margen de la zona)

// -------------------------------------------------------------------
// --- Variables del Temporizador y Estado (MÁQUINA DE ESTADOS) ---
// -------------------------------------------------------------------
unsigned long tiempoInicio = 0;   // [1. Variables de Estado] Momento en que el jugador entró (millis())
bool jugadorEnZona = false;       // [1. Variables de Estado] Estado: ¿El tiempo está contando?
unsigned long tiempoMaximo = 0;    // [1. Variables de Estado] El "High Score" (máxima duración en ms)

// Variables para el timing NO BLOQUEANTE de Log y Botones
unsigned long tLog = 0;           
unsigned long tBtn = 0;           
const int DEBOUNCE_DELAY = 200;   // 200ms para antirrebote de botones

// Declaración de función auxiliar
void printZona(); 


void setup() {
  pinMode(LED, OUTPUT);
  // Los botones usan pull-up interno: LOW = presionado
  pinMode(BTN_UP,   INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println(F("--- JUEGO 'MANTÉN LA ZONA' INICIADO ---"));
  printZona();
}

void loop() {
  unsigned long now = millis();

  // -------------------------------------------------------------------
  // 1. LECTURA Y CONVERSIÓN DE TEMPERATURA
  // -------------------------------------------------------------------
  int adc = analogRead(PIN_NTC); // [Lectura ADC]
  float Vout = (adc / 1023.0) * VREF; // [Lectura ADC -> Voltaje]
  if (Vout < 0.005) Vout = 0.005; 

  float Rntc = R1 * (VREF - Vout) / Vout; // [Calcular RNTC]
  
  float lnR = log(Rntc);
  float invT = A + B*lnR + C*pow(lnR, 3); // [Temperatura (Steinhart–Hart)]
  float tempK = 1.0 / invT;
  float tempC = tempK - 273.15;

  // [Zona Objetivo] Comprueba si la temperatura está en el rango
  bool condicionZona = (tempC >= center - window && tempC <= center + window);


  // -------------------------------------------------------------------
  // 2. LÓGICA DEL TEMPORIZADOR (Máquina de Estados)
  // -------------------------------------------------------------------
  if (condicionZona && !jugadorEnZona) {
    // [2. Detección de Inicio] Entra a la Zona
    jugadorEnZona = true;
    tiempoInicio = now; 
    Serial.println(F("\n-> ¡JUGADOR EN LA ZONA! Tiempo iniciado."));

  } else if (!condicionZona && jugadorEnZona) {
    // [3. Detección de Fin] Sale de la Zona
    
    unsigned long duracion = now - tiempoInicio;
    jugadorEnZona = false;
    
    // Actualiza el High Score
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


  // -------------------------------------------------------------------
  // 3. LED INDICADOR (Feedback Visual No Bloqueante)
  // -------------------------------------------------------------------
  // [LED indicador]
  if (condicionZona) {
    digitalWrite(LED, HIGH); // → Dentro: encendido fijo
  } else if (tempC < center - window) {
    bool blink = ((now / 500) % 2) == 0; // → Frío: parpadeo lento
    digitalWrite(LED, blink ? HIGH : LOW);
  } else {
    bool blink = ((now / 100) % 2) == 0; // → Caliente: parpadeo rápido
    digitalWrite(LED, blink ? HIGH : LOW);
  }

  // -------------------------------------------------------------------
  // 4. SALIDA SERIAL (Log y Conteo en Tiempo Real)
  // -------------------------------------------------------------------
  if (now - tLog > 500) {
    tLog = now;
    Serial.print(F("Temp=")); Serial.print(tempC, 1); Serial.print(F(" °C"));
    Serial.print(F(" | Zona: [")); Serial.print(center - window);
    Serial.print(F(" a ")); Serial.print(center + window); Serial.print(F("]"));
    
    if (jugadorEnZona) {
        // [4. Mostrar el Tiempo Actual]
        unsigned long tiempoActual = (now - tiempoInicio) / 1000;
        Serial.print(F(" | CONTEO: ")); Serial.print(tiempoActual); Serial.print(F(" s"));
    }
    Serial.println(); // [Salida Serial] Log
  }

  // -------------------------------------------------------------------
  // 5. BOTONES (Ajuste de Dificultad)
  // -------------------------------------------------------------------
  // [Botones] Lógica de antirrebote no bloqueante
  if (now - tBtn > DEBOUNCE_DELAY) {
      if (digitalRead(BTN_UP) == LOW) {
          center += 1;
          printZona();
          tBtn = now; 
      }
      if (digitalRead(BTN_DOWN) == LOW) {
          center -= 1;
          printZona();
          tBtn = now;
      }
  }
}

// Función auxiliar para imprimir la zona objetivo
void printZona() {
  Serial.print(F("Nueva zona objetivo: "));
  Serial.print(center - window); Serial.print(F(" a "));
  Serial.print(center + window); Serial.println(F(" °C"));
}
