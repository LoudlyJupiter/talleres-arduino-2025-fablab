# Proyecto Arduino – 3 Clases Prácticas

Este repositorio contiene **3 prácticas con Arduino** pensadas para principiantes.  
Cada clase introduce nuevos componentes y conceptos de programación.

---

### Clase 1: Semáforo con LEDs y servomotor
- Simulación de un semáforo básico con 3 LEDs (rojo, amarillo y verde).
- Uso de `digitalWrite()` y `delay()` para controlar las luces.
- Se ingresa el ángulo al monitor serial y el servomotor se mueve según el ángulo y prende un led según el ángulo (0-180).
- Objetivo: comprender salidas digitales y secuencias.

---

### Clase 2: Juego "Mantén la Zona de Luz" (LDR + LED + Botones) 💡
Esta clase sustituye el sensor de temperatura por el LDR, transformando el juego en un desafío de control de luz.

- **Sensor LDR** (Fotorresistor) como entrada analógica (montado en un divisor de voltaje).
- **Conversión de Resistencia:** Se usa la fórmula del divisor de voltaje para calcular el valor de resistencia ($\mathbf{R_{LDR}}$) a partir del voltaje leído, eliminando la ecuación Steinhart–Hart.
- **Zona Objetivo:** Definida en **Ohms** ($\Omega$).
- **LED Indicador (Luz de Advertencia):**
    - **Apagado Fijo:** La resistencia está **DENTRO** de la zona (ganando tiempo).
    - **Parpadeo lento:** Resistencia **muy alta** (demasiada oscuridad).
    - **Parpadeo rápido:** Resistencia **muy baja** (demasiada luz).
- **Botones:** Control de dificultad no bloqueante para mover la resistencia central de la zona objetivo ($\uparrow / \downarrow$).
- **Objetivo:** Mantener el LED apagado el mayor tiempo posible controlando la luz y practicar el **timing no bloqueante** (`millis()`) y el **antirrebote** de botones.

---

### Clase 3: Detector ultrasónico con servo
- Uso del sensor **HC-SR04** para medir distancias.
- Montaje en un **servo motor** para escanear en un ángulo (0–180°).
- Detección de objetos y activación de alarma (LED/buzzer).
- Objetivo: construir un **radar sencillo** que muestre distancias en tiempo real.
