/*
 * Este programa controla un RELÉ de 4 canales
 * 
 * Por: Cristina Sánchez Saldaña
 *      Rogelio Vázquez Nieves
 * Fecha: 14 de junio de 2022
 * 
 * 
 * RELÉ                          ESP32
 * VCC --------------------------- VIC (5 VOLTS)
 * GND --------------------------- GND
 * IN1 --------------------------- PIN 33
 * IN2 --------------------------- PIN 25
 * IN3 --------------------------- PIN 26
 * IN4 --------------------------- PIN 27
 * 
 * O pueden invertirse el orden de los pines como se desee
*/



//Definiciones
#define RELAY_ON 0   //define una constante de activacion de relé, los relés se activan con nivel BAJO (0)
#define RELAY_OFF 1


//condicones iniciales
void setup () {
  
  //Asegurar nivel ALTO en cada entrada de rele
  digitalWrite (25, RELAY_OFF); 
  digitalWrite (26, RELAY_OFF); 
  digitalWrite (27, RELAY_OFF);  
  digitalWrite (33, RELAY_OFF);  
  
  //Definir los pines como salida
  pinMode (25, OUTPUT);
  pinMode (26, OUTPUT);
  pinMode (27, OUTPUT);
  pinMode (33, OUTPUT);
  
} //Fin setup()


//Ciclo que se enstara ejecutando constantemente
void loop () {
      digitalWrite (25, RELAY_ON);     //Activa relé 1
      delay (2000);                    //lo mantiene encendido durante 2 segundos
      digitalWrite (25, RELAY_OFF);    //Desactiva relé 1
      delay (2000);                    //lo mantiene apagado durante 2 segundos, y despues continua con la siguiente instruccion

      digitalWrite (26, RELAY_ON);     //Activa relé 2
      delay (2000);                    
      digitalWrite (26, RELAY_OFF);    //Desactiva relé 2
      delay (2000);                    

      digitalWrite (27, RELAY_ON);     //Activa relé 2
      delay (2000);                    
      digitalWrite (27, RELAY_OFF);    //Desactiva relé 2
      delay (2000);     

      digitalWrite (33, RELAY_ON);     //Activa relé 2
      delay (2000);                    
      digitalWrite (33, RELAY_OFF);    //Desactiva relé 2
      delay (2000);     
      
} //Fin loop()
   
 
