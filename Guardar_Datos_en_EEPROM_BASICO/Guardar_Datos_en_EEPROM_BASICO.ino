/*Este programa incluye las intrucciones basicas
 *´para leer 24 datos del sensor de temperatura mabiental
 * y almacenarlos en la EEPROM, para no perderlos aun 
 * cuando se desenergize el modulo ESP32
 * 
 * Por: Cristina Sánchez Saldaña
 *      Rogelio Vázquez Nieves
 * Fecha: 14 de junio de 2022
*/

// librerias
#include <EEPROM.h>   // libreria que incluye la lectura y escritura desde la memoria flash
#include <DHT.h>


#define EEPROM_SIZE 112   // definir el numero de byte a los que se desea accesar
                          // float requiere 4 bytes cada uno.
#define  DHTPIN  4        // pin en el que se conecta el sensor de temperatura      
#define  DHTTYPE DHT21 

//Objetos
DHT dht(DHTPIN, DHTTYPE);

// Constantes
const int SensorPin = 4;    // numero de pin al que se conecta el sensor 

//Variables  
int direccion = 0;            //direccion de inicio
float temp_21;               //guarda el valor registrado de temperatura por el sensor    
int No_Lecturas_Temp = 24 ;  //cantidad de lecturas requeridas en el lapso de un dia
int i = 0;                   //contador




//se ejecutara una sola vez el codigo para no utilizar la escritura y lectura de la EEPROM innecesariamente
void setup() { 
    Serial.begin(9600);
    delay(2000);
    int direccion = 0;         //direccion de inicio
    EEPROM.begin(EEPROM_SIZE); //se inicializa la memoria reservada para la EEPROM
    Serial.println(F("DHTxx test!"));
    dht.begin();
    for (i=0; i < No_Lecturas_Temp; i ++){ //inicia el ciclo de 24 lecturas de temperatura cada 100mmilisegundos
          temp_21 = dht.readTemperature();
          float f = dht.readTemperature(true);
         
          if(isnan(temp_21)||isnan(f)){
             Serial.println(F("Falla al leer el Sensor DHT"));
          }
         
          float hic = dht.computeHeatIndex(temp_21, false);
          Serial.print(F("Temperatura (°C) de sensor: "));
          Serial.println(temp_21);
          EEPROM.put(direccion, temp_21);    //escribe "put" en la memoria EEPROM en la direccion indicada el dato obtenido del sensor
          delay(100);
          EEPROM.commit();                   //cierra la operación de escritura
          direccion += sizeof(float);        //incrementa la direccion de memoria, la cantidad necesaria para no sobreescribir los datos
                                             //en este caso el dato float ocupa 4 Bytes
          delay(2900); 
      }
       
       readTempEEPROM ();     //ejecuta la funcion de lectura de los datos almacenados en la memoria EEPROM
}


//loop() no contiene codigo, ya que este codigo se ejecutara solo una sola vez
//y no hacer uso innecesario de las lecturas y escrituras de la EEPROM

void loop() {
      
 
}



//Funciones

//******************************************************
//Funcion de escritura en la EEPROM, puede utilizarse en lugar del "put"
void writeTempEEPROM(int direcion, float temp_21){   
   EEPROM.put(direccion, temp_21);         //escritura en la EEPROM "put", direccion y dato a registrar
   delay(100);
   EEPROM.commit();                        //cierre de la escritura en la EEPROM
   direccion += sizeof(float);             //incremetar la dereccion la cantidad necesaria de acuerdo al dato almacenado
}


//****************************************************************
//Funcion de lectura de la EEPROM
float readTempEEPROM (){
       direccion = 0;
       float valor;
       for (i = 0; i < No_Lecturas_Temp; i ++){       //se leeran 24 datos de la EEPROM
       Serial.println(EEPROM.get(direccion, valor));  //"get" obtiene el valor almacenado en la direccion indicada
        direccion += sizeof(float);
       }
}
