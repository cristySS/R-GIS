/*
 * Este programa monitorea el valor de la humedad del suelo de 3 secciones 
 * Código para Sensor Capasitivo Resistivo
 * 
 * Por: Cristina Sánchez Saldaña
 *      Rogelio Vázquez Nieves
 * Fecha: 14 de junio de 2022
 * 
 * CONEXIONES
 * SENSOR               ESP32
 * Amarillo ------------ SENSORPIN_n
 * Rojo ---------------- 5V
 * Negro --------------- GND
*/

//Constantes
const int SENSORPIN_1 = 34; //pin en el que se conectara la salida A0 del Sensor al ESP32
const int SENSORPIN_2 = 36; //pin en el que se conectara la salida A0 del Sensor al ESP32
const int SENSORPIN_3 = 39; //pin en el que se conectara la salida A0 del Sensor al ESP32


//Variables
int humedad_1, humedad_2, humedad_3;     //variables ue guardaran la lectura de la humedad de cada sensor 1, 2 y 3

//Configuraciones iniciales, se ejecutan una sola vez
void setup() {
   Serial.begin(9600);  //velocidad de conexión del puerto serial de salida
}


//Intrucciones que se estaran repitiendo constantemente 
void loop() 
{
 // soilMoistureValue = analogRead(A0);  //put Sensor insert into soil
   humedad_1 = analogRead(SENSORPIN_1);
   Serial.println("Sensor 1:");
   Serial.println(humedad_1);
    
   humedad_2 = analogRead(SENSORPIN_2);
   Serial.println("Sensor 2:");
   Serial.println(humedad_2);
    
   humedad_3 = analogRead(SENSORPIN_3);
   Serial.println("Sensor 3:");
   Serial.println(humedad_3);
   delay(3000);  //hace una lectura continua de los tres sensores, se detiene 3 segundos y vuelve a relaizar la lectura
  
}
