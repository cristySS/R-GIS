/*
 * como usar el sensor DHT21 con ESP32 
 * Por: Cristina Sánchez Saldaña
 *      Rogelio Vázquez Nieves
 * Fecha: 14 de junio de 2022
 * 
 * 
 * SENSOR DHT21                           ESP32
 * 
 * ROJO      ------resistencia----------- VCC
 * NEGRO     ---------------------------  GND
 * AMARILLO  ------resistencia----------  DHTPIN
 * 
 * la resistencia requerida es de 10 K Ohms
 */

// Bibliotecas
#include "DHT.h"   //libreria a utilizar DHT sensor library


// Constantes
#define DHTPIN 4          // a que pin se conectara el DHT21 al ESP32
#define DHTTYPE DHT21      //tipo de sensor que se estara usando DHT21 (AM2301)

// Variables
float hum_21;            //almacena el valor de la humedad leida por el sensor DHT21
float temp_21;           //almacena el valor de la temperatura leida por el sensor DHT21

// Definición de objetos
DHT dht(DHTPIN, DHTTYPE);      //se crea el objeto a usar del sensor

// Condiciones iniciales - Se ejecuta sólo una vez al energizar
void setup() {                           // Inicio de void setup ()
  Serial.begin(9600);                    //indica el indice de comunicacion conla computadora y cualquier dispositivo a una veli¿ocidad de comunicacion serial de 9600 bits por segundo (baudios)
  Serial.println(F("DHTxx test!"));      //mensaje de prueba de conectividad con el sensor

  dht.begin();                           //iniciaciacion del objeto  
}// Fin de void setup

// Cuerpo del programa - Se ejecuta constantemente
void loop() {// Inicio de void loop
  delay(2000);                     //pausa del programa antes de comenzar a obtener datos del sensor en (milisegundos)
  //lectura de humedad o temperatura
  //el sensor lee cada 2 segundos
  hum_21 = dht.readHumidity();     //lectura de humedad
  temp_21 = dht.readTemperature(); //lectura de temperatura

  float f = dht.readTemperature(true);

  //revision si existe falla en la lectura del sensor
  if (isnan(hum_21) || isnan(temp_21) || isnan(f)) {
    Serial.println(F("Falla al leer el sensor DHT"));
    return;
  } // fin de if

  float hif = dht.computeHeatIndex(f, hum_21);
  float hic = dht.computeHeatIndex(temp_21, hum_21, false);

  Serial.print(F("Humedad: "));           //imprime en el serial el mensaje, combinado con la macro F() ayuda a ahorrar memoria SRAM, al mover la cadena a la memoria FLASH
  Serial.print(hum_21);                   //imprime el valor de la variable
  Serial.print(F("%    Temperatura:"));
  Serial.print(temp_21);
  Serial.println(F("°C"));
  
  

}// Fin de void loop

// Funciones del usuario
