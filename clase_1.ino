#include <Servo.h>

Servo miServo;

const int ledAzul = 2;
const int ledAmarillo = 3;
const int ledRojo = 4;
const int pinServo = 9;

void setup() {
  miServo.attach(pinServo);

  pinMode(ledAzul, OUTPUT);
  pinMode(ledAmarillo, OUTPUT);
  pinMode(ledRojo, OUTPUT);

  Serial.begin(9600);
  apagarTodosLosLEDs();

  Serial.println("Introduce un ángulo entre 0 y 180 grados.");
}

void loop() {
  if (Serial.available()) {
    int angulo = Serial.parseInt();

    if (angulo >= 0 && angulo <= 180) {
      miServo.write(angulo);

      if (angulo < 60) {
        encenderLED(ledAzul);
      } else if (angulo <= 120) {
        encenderLED(ledAmarillo);
      } else {
        encenderLED(ledRojo);
      }

      Serial.print("Ángulo recibido: ");
      Serial.println(angulo);
    } else {
      Serial.println("Ángulo inválido. Debe estar entre 0 y 180.");
      while (Serial.available()) {
        Serial.read();
      }
    }
  }
}

void encenderLED(int led) {
  apagarTodosLosLEDs();
  digitalWrite(led, HIGH);
}

void apagarTodosLosLEDs() {
  digitalWrite(ledAzul, LOW);
  digitalWrite(ledAmarillo, LOW);
  digitalWrite(ledRojo, LOW);
}
