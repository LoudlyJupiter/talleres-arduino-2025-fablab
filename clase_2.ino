// -------------------------------------------------------------------
// --- Pines y Constantes de Hardware ---
// -------------------------------------------------------------------
const int PIN_LDR       = A0;    // Nodo del divisor (LDR ↔ R1)
const int LED           = 9;     // LED indicador de estado y zona
const int BTN_UP        = 2;     // Botón para subir la RESISTENCIA objetivo
const int BTN_DOWN      = 3;     // Botón para bajar la RESISTENCIA objetivo

// Constantes eléctricas / ADC
const float VREF        = 5.0;     // Voltaje de referencia del ADC (Arduino UNO típico)
const float R1          = 10000.0; // Resistencia fija del divisor (10k Ohms)

// Zona objetivo (Ajustable con botones, ahora en Ohms)
float center = 10000.0; // Ohms (Centro de la zona de juego) - Ajusta este valor en la práctica
float window = 2000.0;  // ±Ohms (Margen de la zona)

// -------------------------------------------------------------------
// --- Variables del Temporizador y Estado (MÁQUINA DE ESTADOS) ---
// -------------------------------------------------------------------
unsigned long tiempoInicio = 0;    
bool jugadorEnZona = false;        
unsigned long tiempoMaximo = 0;     

// Variables para el timing NO BLOQUEANTE de Log y Botones
unsigned long tLog = 0;            
unsigned long tBtn = 0;            
const int DEBOUNCE_DELAY = 200;    

// Declaración de función auxiliar
void printZona(); 


void setup() {
  pinMode(LED, OUTPUT);
  pinMode(BTN_UP,   INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println(F("--- JUEGO 'MANTÉN LA ZONA' (LDR) INICIADO ---"));
  printZona();
}

void loop() {
  unsigned long now = millis();

  // 1. LECTURA Y CONVERSIÓN DE RESISTENCIA (LDR)
  int adc = analogRead(PIN_LDR);
  float Vout = (adc / 1023.0) * VREF;
  if (Vout < 0.005) Vout = 0.005; 
  float Rldr = R1 * (VREF - Vout) / Vout;
  bool condicionZona = (Rldr >= center - window && Rldr <= center + window);


  // -------------------------------------------------------------------
  // 2. LÓGICA DEL TEMPORIZADOR Y CONTROL DE LED FIJO
  // -------------------------------------------------------------------
  if (condicionZona && !jugadorEnZona) {
    // Entra a la Zona
    jugadorEnZona = true;
    tiempoInicio = now; 
    digitalWrite(LED, HIGH); // <-- FORZAMOS EL LED A FIJO (ENCENDIDO)
    Serial.println(F("\n-> ¡JUGADOR EN LA ZONA! Tiempo iniciado."));

  } else if (!condicionZona && jugadorEnZona) {
    // Sale de la Zona
    unsigned long duracion = now - tiempoInicio;
    jugadorEnZona = false;
    digitalWrite(LED, LOW); // <-- FORZAMOS EL LED A APAGADO (ANTES DEL PARPADEO)

    // Actualiza el High Score y reporte
    if (duracion > tiempoMaximo) {
      tiempoMaximo = duracion;
    }
    Serial.print(F("<- ¡ZONA PERDIDA! Duración: "));
    Serial.print(duracion / 1000.0); Serial.println(F(" s"));
    Serial.print(F("¡TIEMPO MÁXIMO (HS) REGISTRADO: "));
    Serial.print(tiempoMaximo / 1000.0); Serial.println(F(" s!"));
    Serial.println(F("----------------------------------------"));
  }


  // -------------------------------------------------------------------
  // 3. LED INDICADOR (Solo maneja el parpadeo si NO está en la zona de juego)
  // -------------------------------------------------------------------
  if (!jugadorEnZona) {
      if (Rldr > center + window) {
        // Resistencia muy ALTA (OSCURO)
        bool blink = ((now / 500) % 2) == 0; // → Oscuro: parpadeo lento
        digitalWrite(LED, blink ? HIGH : LOW);
      } else if (Rldr < center - window) {
        // Resistencia muy BAJA (CLARO)
        bool blink = ((now / 100) % 2) == 0; // → Claro: parpadeo rápido
        digitalWrite(LED, blink ? HIGH : LOW);
      } else {
        // En este punto, no estamos en zona pero estamos cerca de la ventana (apagar para evitar glitch)
        digitalWrite(LED, LOW);
      }
  }


  // 4. SALIDA SERIAL (Log y Conteo en Tiempo Real)
  if (now - tLog > 500) {
    tLog = now;
    Serial.print(F("R_LDR=")); Serial.print(Rldr); Serial.print(F(" Ohm"));
    Serial.print(F(" | Zona: [")); Serial.print(center - window);
    Serial.print(F(" a ")); Serial.print(center + window); Serial.print(F("]"));
    
    if (jugadorEnZona) {
        unsigned long tiempoActual = (now - tiempoInicio) / 1000;
        Serial.print(F(" | CONTEO: ")); Serial.print(tiempoActual); Serial.print(F(" s"));
    }
    Serial.println();
  }

  // 5. BOTONES (Ajuste de Dificultad)
  if (now - tBtn > DEBOUNCE_DELAY) {
      if (digitalRead(BTN_UP) == LOW) {
          center += 100.0;
          printZona();
          tBtn = now; 
      }
      if (digitalRead(BTN_DOWN) == LOW) {
          center -= 100.0;
          printZona();
          tBtn = now;
      }
  }
}

// Función auxiliar
void printZona() {
  Serial.print(F("Nueva zona objetivo: "));
  Serial.print(center - window); Serial.print(F(" a "));
  Serial.print(center + window); Serial.println(F(" Ohm"));
}
