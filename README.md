# 🚀 Proyecto Arduino – 3 Clases Prácticas

Este repositorio contiene **3 prácticas con Arduino** pensadas para principiantes.  
Cada clase introduce nuevos componentes y conceptos de programación.

---

## 📚 Contenido de las clases

### 🔴 Clase 1: Semáforo con LEDs
- Simulación de un semáforo básico con 3 LEDs (rojo, amarillo y verde).
- Uso de `digitalWrite()` y `delay()` para controlar las luces.
- Objetivo: comprender salidas digitales y secuencias.

---

### 🌡️ Clase 2: Juego “Mantén la Zona” (NTC + LED + Botones)
- Sensor **NTC** como entrada analógica (divisor de voltaje).
- Conversión de temperatura usando la ecuación **Steinhart–Hart**.
- Definición de una **zona objetivo** (ejemplo: 30 ± 2 °C).
- LED indicador:
  - 🔵 Fijo → dentro de la zona.
  - ❄️ Parpadeo lento → frío.
  - 🔥 Parpadeo rápido → caliente.
- Botones para mover la zona ↑ / ↓.
- Objetivo: mantener el LED fijo el mayor tiempo posible.

---

### 📡 Clase 3: Detector ultrasónico con servo
- Uso del sensor **HC-SR04** para medir distancias.
- Montaje en un **servo motor** para escanear en un ángulo (0–180°).
- Detección de objetos y activación de alarma (LED/buzzer).
- Objetivo: construir un **radar sencillo** que muestre distancias en tiempo real.

---

## 📂 Estructura del repositorio

Proyecto-Arduino/
├── Clase1_Semaforo/
│ └── Semaforo.ino
├── Clase2_MantenLaZona/
│ └── MantenLaZona.ino
├── Clase3_UltrasonicoServo/
│ └── UltrasonicoServo.ino
└── README.md


---

## 🛠️ Materiales generales
- Arduino UNO (o compatible)
- Protoboard y cables
- LEDs + resistencias
- Pulsadores
- NTC + resistencia fija (10kΩ)
- Sensor ultrasónico HC-SR04
- Servo motor SG90 (o similar)
- (Opcional) buzzer para alarma

---

## 🏆 Objetivo final
Cada clase suma complejidad:  
1. **Clase 1:** salidas digitales (LEDs).  
2. **Clase 2:** entradas analógicas + lógica de juego.  
3. **Clase 3:** sensores de distancia + actuadores (servo).  

