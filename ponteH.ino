
/* Práctica Simulacion de un puente H con relés - Brais Gude Martínez
Fecha: 01/03/2026

Se simula un puente H con 4 relés para controlar un motor DC.
El sistema tiene 5 estados posibles que se representan en la tabla de verdad de abajo.
Se controla mediante un pulsador. Cada pulsación cambia el estado del motor siguiendo la secuencia:

 * TABLA DE VERDAD PUENTE H
 * -------------------------------------------------------
 * S1=TopIzq  S2=BotIzq  S3=TopDer  S4=BotDer
 *
 * S1 | S2 | S3 | S4 | Estado motor
 * ---|----|----|----|----------------------------
 *  0 |  0 |  0 |  0 | (-1) Freno por inercia     ✅  
 *  1 |  0 |  0 |  1 | (a)  Giro antihorario      ✅  
 *  0 |  1 |  0 |  1 | (b)  Freno inmediato (GND) ✅  
 *  0 |  1 |  1 |  0 | (c)  Giro horario          ✅  
 *  1 |  0 |  1 |  0 | (d)  Freno inmediato (VCC) ✅ 
 *  1 |  1 |  x |  x | Cortocircuito alimentación ❌  
 *  x |  x |  1 |  1 | Cortocircuito alimentación ❌  
 *  1 |  0 |  0 |  0 | Motor en vacío (un lado)   ❌  
 *  0 |  1 |  0 |  0 | Motor en vacío (un lado)   ❌  
 *
 * Diferencia entre tipos de frenado:
 * - Freno por inercia: todos los relés abiertos, el motor gira
 *   libremente hasta detenerse por rozamiento.
 * - Freno inmediato: los dos terminales del motor se cortocircuitan
 *   entre sí (ambos a GND o ambos a VCC), generando una corriente
 *   de frenado que detiene el motor bruscamente.
 */


// Relés
constexpr int K1 = 2;
constexpr int K2 = 3;
constexpr int K3 = 4;
constexpr int K4 = 5;

constexpr int S1 = 9; // Pulsador

// Tiempos
constexpr unsigned long FrenadoInercia = 15000; // El que tarda en detenerse por inercia: 15s
constexpr unsigned long Debounce = 50;    // Debounce

// Guardado del estado actual del motor
int estadoActual = -1; // -1 Freno inercia, 0 giro antihorario, 1 freno inmediato, 2 giro horario, 3 freno inmediato


// Control de transición automática a inercia
bool enFrenoInmediato              = false; // flag para indicar si estamos en freno inmediato
unsigned long tiempoFrenoInmediato = 0; // Guarda el ultimo tiempo de freno inmediato

// Debounce del pulsador para garantizar solo una transición por pulsación
bool estadoPulsadorAnterior      = LOW;
unsigned long tiempoUltimoCambio = 0;
bool pulsacionProcesada          = false;

// Tabla de la verdad aplicada con switch-case
void aplicarEstado(int estado) {
  bool k1 = false, k2 = false, k3 = false, k4 = false;
  String descripcion;

  switch (estado) {
    case -1: // estado A: Freno por inercia
      k1 = false; k2 = false; k3 = false; k4 = false;
      descripcion      = "Estado (-1) freno por inercia ";
      enFrenoInmediato = false;
      break;
    case 0: // estado B: Giro antihorario
      k1 = true;  k2 = false; k3 = false; k4 = true;
      descripcion      = "Estado (a)  giro antihorario  ";
      enFrenoInmediato = false;
      break;
    case 1: // estado C: Freno inmediato 
      k1 = false; k2 = true;  k3 = false; k4 = true;
      descripcion          = "Estado (b)  freno inmediato   ";
      enFrenoInmediato     = true;
      tiempoFrenoInmediato = millis();
      break;
    case 2: // estado D: Giro horario
      k1 = false; k2 = true;  k3 = true;  k4 = false;
      descripcion      = "Estado (c)  giro horario      ";
      enFrenoInmediato = false;
      break;
    case 3: // estado E: Freno inmediato
      k1 = true;  k2 = false; k3 = true;  k4 = false;
      descripcion          = "Estado (d)  freno inmediato   ";
      enFrenoInmediato     = true;
      tiempoFrenoInmediato = millis();
      break;
  }

// Aplicacion del estado a los relés. Ej: El K1, k1 ? es como un if else.
  digitalWrite(K1, k1 ? HIGH : LOW);
  digitalWrite(K2, k2 ? HIGH : LOW);
  digitalWrite(K3, k3 ? HIGH : LOW);
  digitalWrite(K4, k4 ? HIGH : LOW);
// información por serial del estado actual
  Serial.print(descripcion);
  Serial.print("-- Salidas K1K2K3K4: "); 
  Serial.print(k1); Serial.print(k2); Serial.print(k3); Serial.println(k4);
}

void setup() {
  Serial.begin(9600);

  pinMode(K1, OUTPUT);
  pinMode(K2, OUTPUT);
  pinMode(K3, OUTPUT);
  pinMode(K4, OUTPUT);

  pinMode(S1, INPUT);

  aplicarEstado(-1); // aplica el freno por inercia como estado inicial
}


void loop() {

  // En este loop se controla la transicion a inercia después de un freno inmediato.
  // Si estamos en freno inm. y ha pasado el tiempo de frenado por inercia, se cambia al estado de freno por inercia.
  if (enFrenoInmediato && millis() - tiempoFrenoInmediato >= FrenadoInercia) {
    estadoActual = -1;
    aplicarEstado(-1);
  }

  // Aqui se controla el debounce del pulsador para garantizar 1 sola pulsacion. Es redundante con el control de pulsacionProcesada.
  bool lectura = digitalRead(S1);

  // Si el estado del pulsador cambia se actualiza el tiempo del ultimo cambio y el estado anterior del pulsador
  if (lectura != estadoPulsadorAnterior) {
    tiempoUltimoCambio     = millis();
    estadoPulsadorAnterior = lectura;
  }

  // Si ha pasado el tiempo de debounce desde el ultimo cambio, se procesa la pulsacion. 
  // Si el estado anterior del pulsador era HIGH y no se ha procesado la pulsacion, se cambia el estado del motor.
  if (millis() - tiempoUltimoCambio >= Debounce) {
    if (estadoPulsadorAnterior == HIGH && !pulsacionProcesada) {
      pulsacionProcesada = true;
      if (estadoActual == -1) estadoActual = 0;
      else                    estadoActual = (estadoActual + 1) % 4;
      aplicarEstado(estadoActual);
    }
    if (estadoPulsadorAnterior == LOW) { // Si el pulsador se suelta, se resetea la variable de control de pulsacion procesada para permitir una nueva transición en la siguiente pulsación.
      pulsacionProcesada = false;
    }
  }
}