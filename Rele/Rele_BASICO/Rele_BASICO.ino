/* Este programa permite conectar un Modulo Rele
* Conectamos el modulo de la siguiente forma:
* MODULO       ESP32
* GND -------- GND
* VDD -------- 5V
* IN --------- 02
* 
* Por: Cristina Sánchez Saldaña
*      Rogelio Vázquez Nieves
* Fecha: 14 de junio de 2022 
*/

//Constantes
 const int myRelee = 2; // Declaramos la salida del relee

 
//Condicones iniciales
 void setup(){
     pinMode(myRelee, OUTPUT);  // Declaramos que el pin sera salida
     Serial.begin(9600);  // Preparamos el puerto serie, velocidad de salida
 }  //fin setup()


//Inicio loop(), se ejecutara continuamente
 void loop(){
      // LOW encendido HIGH apagado
      digitalWrite(myRelee, HIGH);
      Serial.println("Relee apagado");
      delay (2000);
      digitalWrite(myRelee, LOW);
      Serial.println("Relee encendido");
      delay (2000);
 } //fin  loop()
