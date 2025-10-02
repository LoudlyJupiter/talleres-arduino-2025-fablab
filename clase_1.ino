const int LED_ROJO     = 2;
const int LED_AMARILLO = 3;
const int LED_VERDE    = 4;

const unsigned long T_VERDE    = 5000;  // 5 segundos
const unsigned long T_AMARILLO = 1500;  // 1.5 segundos
const unsigned long T_ROJO     = 5000;  // 5 segundos

void setup() {
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
}

void apagarTodo() {
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_VERDE, LOW);
}

void faseVerde() {
  apagarTodo();
  digitalWrite(LED_VERDE, HIGH);
  delay(T_VERDE);
}

void faseAmarillo() {
  apagarTodo();
  digitalWrite(LED_AMARILLO, HIGH);
  delay(T_AMARILLO);
}

void faseRojo() {
  apagarTodo();
  digitalWrite(LED_ROJO, HIGH);
  delay(T_ROJO);
}

void loop() {
  faseVerde();
  faseAmarillo();
  faseRojo();
}
