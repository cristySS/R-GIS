/*
 * Este programa toma la Fecha y hora de acuerdo a la zona horaria de la CDMX, México
 * Registra las temperaturas amabientales dadas por el DHT21 en un perido de 24 horas
 * registradas cada hora, almacenadas en la EEPROM del ESP32
 * Monitorea el valor de la humedad del suelo de 3 secciones y si es mayor al valor deseado
 * indica que sección requiere recuperar la evapotranspiración calculada
 * Calcula la Temperatura minima, Temperatura MAXIMA y Temperatura Promedio
 * Calcula la EVAPOTRANSPIRACION del pasto que será necesaria recuperar con el riego
 * Calcula la lamina de riego y tomando en cuenta el area a regar, determina el tiempo de 
 * riego necesario para recuperar el agua perdida por la Evapotranspiración.
 * La apertura y cierre de electrovalvulas, asi como el encendido de la bomba es controlado
 * por el RELÉ
 * 
 * Por: Cristina Sánchez Saldaña
 *      Rogelio Vázquez Nieves
 * Fecha: 14 de junio de 2022
*/

//Librerías
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DHT.h>
#include "math.h"
#include <EEPROM.h>

// Replace with your network credentials
const char* ssid     = "INFINITUM5853";// "IZZI-8F7C"; //"GTYV"; //"ARRIS-8F7C"; //
const char* password = "JKcuTY9G4C"; // "C85261838F7C"; //"V1ncul4cI0n*#2021"; // "C85261838F7C"; //

// Define NTP Client para obtener la fecha/hora
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);


//CONSTANTES
#define EEPROM_SIZE 112
#define  DHTPIN  4           
#define  DHTTYPE DHT21 
#define RELAY_ON 0   //define una constante de activacion de relé, los relés se activan con nivel BAJO (0)
#define RELAY_OFF 1


const float GSC = 0.082;                  //constante solar en  MJ/m  por min
const float EFICIENCIA = 0.80;             //eficiencia de riego de acuerdo al diseño hidraulico, en este caso es por aspersores conuna eficiencia del 80-85% de cobertura
const int SENSORPIN_1 = 34;               //pin en el que se conectara la salida A0 del sensor de humedad de suelo ubicado en Seccion 1
const int SENSORPIN_2 = 36;               //pin en el que se conectara la salida A0 del sensor de humedad de suelo ubicado en Seccion 2
const int SENSORPIN_3 = 39;               //pin en el que se conectara la salida A0 del sensor de humedad de suelo ubicado en Seccion 3

// Variables to save date and time
String formattedDate;                                                                    //almacena el formato de la fecha AAAA-MM-DDTHH:MM:SSz
String dayStamp;                                                                         //almacena solo la fecha AAAA-MM-DD
String timeStamp;                                                                         //almacena solo el tiempo HH:MM:SS
int numero_de_dias;                                                                       //acumula los dias transcurridos por los meses completos
int numero_dia;                                                                           // acumula los dias transcurridos del mes vigente
int Total_dias_anyo;                                                                      //almacena el total de dias transcurridos en el año a la fecha actual
float hum_21, temp_21, temp_min, temp_max, avg_temp, sum_temp;                            //variables del sensor DHT21 (humedad, temperatura), temperaturas minima,mmaxima y promedio del dia
float etc, eto, kc, ra, x, dr, ws, ds;                                                    //variables de la Evapotranspiración
float lrb, lrb_m, g, area_de_riego, g_1, g_2, g_3, ga_1, ga_2, ga_3, ta_1, ta_2, ta_3;    //variables para el tiempo de riego
bool riego1 = false, riego2 = false, riego3 = false, regado1 = false, regado2 = false, regado3 = false, regado_TODO = false;                                                             //indica que seccion 1,2  o 3 requiere regarse, alhaber registrado una humedad por encima de la deseda (sequedad), se define float ya que almacenara en la EEPROM 
int direccion;                                                                         //inicio de memoria EEPROM en 0
int No_Lecturas_Temp = 0;                                                                 //cantidad de lecturas al dia del sensor de temperatura ambiental
int i;                                                                                     //contador
float valor;                                                                               //dato guardado en la direccion EEPROM
bool registro = false;                                                                     //indica si la temperatura ya se registro o no en la hora actual
int humedad_1, humedad_2, humedad_3;                                                       //indican la humedad del suelo, de acuerdo a la seccion
int hr_int, min_int;                                                                       // horas y minutos en tipo ENTERO


//Arreglos
//float Temperaturas_DTH21[24];

//Objects
DHT dht(DHTPIN, DHTTYPE);



void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  EEPROM.begin(EEPROM_SIZE);
  direccion = 0;
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

// Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(-17990); //zona horaria de CDMX
  
   Serial.println(F("DHTxx test!"));
   dht.begin();
   
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
}



void loop() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  
   Serial.print("riego1: ");Serial.println(riego1); 
   Serial.print("riego2: ");Serial.println(riego2); 
   Serial.print("riego3: "); Serial.println(riego3); 
   Serial.print("Ya se registro temperatura ambiental en hora exacta en EEPROM: "); Serial.print(registro); Serial.println(" 0 = NO, 1= SI");
   Serial.print("direccion EEPROM: ");Serial.println(direccion);
   Serial.print("Se llevan resgistradas "); Serial.print(No_Lecturas_Temp); Serial.println(" lecturas de temperatura");
  Fecha_Hora();  //Esta función muestra la Fecha y hora actual, llama a la función que calcula
                 // el numero de dias transcurridos y llama la función de registro de temperatura
                 // ambiental con periodos de 1 hora
  
  Serial.print("Total dias del año: ");Serial.println(Total_dias_anyo);
  Toma_Temperatura(hr_int, min_int);
  Humedad_Sensor_1();
  Humedad_Sensor_2();
  Humedad_Sensor_3();
  //delay(6000); //toma la temperatura ambiental cada 3 segundos
  if ((min_int == 0) && (registro == false) && (No_Lecturas_Temp == 0)){     //ciclo para registrar la temperatura ambiental en el minuto 0 de cada hora
       No_Lecturas_Temp = No_Lecturas_Temp +1;   //No_Lecturas_Temp lleva el conteo de cuantos registros de temperatura ambiental se llevan al momento
       EEPROM.put(direccion, temp_21);           //registra la lectura temp ambiental en la direccion de la EEPROM
       delay(100);
       EEPROM.commit();                           //cierra la operacion de escritura
       delay(100);
       Serial.print("Se registro una temperatura de: ");
       Serial.print(EEPROM.get(direccion, valor));
       Serial.print(" en la direccion EEPROM ");
       Serial.println(direccion);
       //direccion += sizeof(float);                //incrementa la direccion de memoria en los bytes necesarios que ocupo el float almacenado
       Serial.print(" a la hora ");
       Serial.println(hr_int);
       registro = true;                         //ya se registro la temperatura durante el minuto 0 de la hora actual
       Serial.print("Se llevan registradas "); Serial.print(No_Lecturas_Temp); Serial.println(" temperaturas ambienatales en la EEPROM");
  }
  if ((min_int == 0) && (registro == false) && (No_Lecturas_Temp >= 1)){     //ciclo para registrar la temperatura ambiental en el minuto 0 de cada hora
       No_Lecturas_Temp = No_Lecturas_Temp +1;   //No_Lecturas_Temp lleva el conteo de cuantos registros de temperatura ambiental se llevan al momento
       direccion += sizeof(float);                   //incrementa la direccion de memoria en los bytes necesarios que ocupo el float almacenado 
       EEPROM.put(direccion, temp_21);           //registra la lectura temp ambiental en la direccion de la EEPROM
       delay(100);
       EEPROM.commit();                           //cierra la operacion de escritura
       delay(100);
       Serial.print("Se registro una temperatura de: ");
       Serial.print(EEPROM.get(direccion, valor));
       Serial.print(" en la direccion EEPROM ");
       Serial.println(direccion);
       Serial.print(" a la hora ");
       Serial.println(hr_int);
       registro = true;                         //ya se registro la temperatura durante el minuto 0 de la hora actual
       Serial.print("Se llevan registradas "); Serial.print(No_Lecturas_Temp); Serial.println(" temperaturas ambienatales en la EEPROM");
  }
  if ((min_int > 1) && (registro == true)){        
           registro = false;                            //con "false", en la siguiente hora al minuto 0 volvera a registrar la temperatura
     }
  if (No_Lecturas_Temp == 24){              //revisa si ya se tienen 24 registros de temperatura ambiental, cubriendo un dia completo
           direccion = 0;                          //la direccion donde se registrara la siguiente temperatura sera nuevamente en la direccion 0
           No_Lecturas_Temp = 0;
        }  
  Serial.print("Necesita riego seccion 1: ");
  Serial.println(riego1); 
  Serial.print("Necesita riego seccion 2: ");
  Serial.println(riego2);
  Serial.print("Necesita riego seccion 3: ");
  Serial.println(riego3);   
  if ((hr_int == 23) && (regado_TODO == false)){ 
           if ((regado1 == false) && (riego1 == true)){                        //riega seccion 1
               //comprobar si en eltranscurso del dia, antes de la hora fijda de riego, la seccion recupero o no la humedad, puede ser a causa de lluvia
               humedad_1 = analogRead(SENSORPIN_1);
               Serial.println("Humedad actual de la Sección 1:");
               Serial.println(humedad_1);
               if (humedad_1 > 2700){
                   Serial.println("requiere riego la seccion 1...");
                   Calculo_TempmMp();                                                   //Esta función calcula la temperatura minima, maxima y promedio
                   Evapotranspiracion(Total_dias_anyo, temp_min, temp_max, avg_temp);  //devuelve la etc
                   Tiempo_de_Riego(EFICIENCIA, etc);                                    //devuelve el tiempo de reiego necesario en cada seccion en minutos
                   Serial.print("Tiempo de riego, Sección 1: ");
                   Serial.print(ta_1, 5);
                   Serial.println(" minutos");
                   int tiempo_riego_1 = ta_1 * 60000;                                   //calcula el tiempo en milisegundos que se mantendra la bomba y electrovalvula de la seccion 1 abierta
                   Serial.print("regando seccion 1...Esto tomará (riego prioritario):");
                   Serial.print(tiempo_riego_1);
                   Serial.println(" milisegundos");
                   digitalWrite (26, RELAY_ON);     //Activa relé 2, que abre la ELECTROVALVULA 1
                   digitalWrite (25, RELAY_ON);     //Activa relé 1, que enciende la BOMBA
                   delay (2000);
                   //delay(Tiempo_riego_1);
                   digitalWrite (25, RELAY_OFF);    //Desactiva relé 1, apaga la BOMBA
                   digitalWrite (26, RELAY_OFF);    //Desactiva relé 2, cierra la ELECTROVALVULA 1
                   regado1 = true;
                   riego1 = false;
                   regado_TODO =true;
               } 
               
               else {
                   Serial.print("la seccion 1 ha recuperado la humedad necesaria: ");
                   Serial.println(humedad_1);
               }
            }
           else {
               if ((regado2 == false) && (riego2 == true)){
                  //riega seccion 2
                   humedad_2 = analogRead(SENSORPIN_2);
                   Serial.println("Humedad actual de la Sección 2:");
                   Serial.println(humedad_2);
                   if (humedad_2 > 2700){
                      Serial.println("requiere riego la seccion 2...");
                      Calculo_TempmMp();                                                   //Esta función calcula la temperatura minima, maxima y promedio
                      Evapotranspiracion(Total_dias_anyo, temp_min, temp_max, avg_temp);  //devuelve la etc
                      Tiempo_de_Riego(EFICIENCIA, etc);                                    //devuelve el tiempo de reiego necesario en cada seccion en minutos
                      Serial.print("Tiempo de riego, Sección 2: ");
                      Serial.print(ta_2, 5);
                      Serial.println(" minutos");
                      int tiempo_riego_2 = ta_2 * 60000;                                   //calcula el tiempo en milisegundos que se mantendra la bomba y electrovalvula de la seccion 1 abierta
                      Serial.print("regando seccion 2...Esto tomará (riego prioritario):");
                      Serial.print(tiempo_riego_2);
                      Serial.println(" milisegundos");
                      digitalWrite (27, RELAY_ON);     //Activa relé 3, que abre la ELECTROVALVULA 2
                      digitalWrite (25, RELAY_ON);     //Activa relé 1, que enciende la BOMBA
                      delay (2000);
                     //delay(Tiempo_riego_2);
                      digitalWrite (25, RELAY_OFF);    //Desactiva relé 1, apaga la BOMBA
                      digitalWrite (27, RELAY_OFF);    //Desactiva relé 3, cierra la ELECTROVALVULA 2
                      regado2 = true;
                      riego2 = false;
                      regado_TODO =true;
                    }
                  }
                  else{
                      if ((regado3 == false) && (riego3 == true)){
                           //riega seccion 3 
                           humedad_3 = analogRead(SENSORPIN_3);
                           Serial.println("Humedad actual de la Sección 3:");
                           Serial.println(humedad_3);
                           if (humedad_3 > 2700){
                              Calculo_TempmMp();                                                   //Esta función calcula la temperatura minima, maxima y promedio
                              Evapotranspiracion(Total_dias_anyo, temp_min, temp_max, avg_temp);  //devuelve la etc
                              Tiempo_de_Riego(EFICIENCIA, etc);                                    //devuelve el tiempo de reiego necesario en cada seccion en minutos
                              Serial.print("Tiempo de riego, Sección 3: ");
                              Serial.print(ta_3, 5);
                              Serial.println(" minutos");
                              int tiempo_riego_3 = ta_3 * 60000;                                   //calcula el tiempo en milisegundos que se mantendra la bomba y electrovalvula de la seccion 1 abierta
                              Serial.print("regando seccion 3...Esto tomará (riego prioritario):");
                              Serial.print(tiempo_riego_3);
                              Serial.println(" milisegundos");
                              digitalWrite (33, RELAY_ON);     //Activa relé 4, que abre la ELECTROVALVULA 3
                              digitalWrite (25, RELAY_ON);     //Activa relé 1, que enciende la BOMBA
                              delay (2000);
                              //delay(Tiempo_riego_3);
                              digitalWrite (25, RELAY_OFF);    //Desactiva relé 1, apaga la BOMBA
                              digitalWrite (33, RELAY_OFF);    //Desactiva relé 4, cierra la ELECTROVALVULA 3
                              regado3 = true;
                              riego3 = false;
                              regado_TODO = true;
                            }  
                           else { //recuperara a evapotranspiracion sin prioridad
                                Calculo_TempmMp();                                                   //Esta función calcula la temperatura minima, maxima y promedio
                                Evapotranspiracion(Total_dias_anyo, temp_min, temp_max, avg_temp);  //devuelve la etc
                                Tiempo_de_Riego(EFICIENCIA, etc);                                    //devuelve el tiempo de reiego necesario en cada seccion en minutos
                                Serial.print("Tiempo de riego, Sección 1: ");
                                Serial.print(ta_1, 5);
                                Serial.println(" minutos");
                                Serial.print("Tiempo de riego, Sección 2: ");
                                Serial.print(ta_2, 5);
                                Serial.println(" minutos");
                                Serial.print("Tiempo de riego, Sección 3: ");
                                Serial.print(ta_3, 5);
                                Serial.println(" minutos");
                                digitalWrite (26, RELAY_ON);     //Activa relé 2, que abre la ELECTROVALVULA 1
                                digitalWrite (25, RELAY_ON);     //Activa relé 1, que enciende la BOMBA
                                delay (2000);
                                //delay(Tiempo_riego_1);
                                digitalWrite (27, RELAY_ON);     //Activa relé 3, que abre la ELECTROVALVULA 2
                                digitalWrite (26, RELAY_OFF);    //Desactiva relé 2, cierra la ELECTROVALVULA 1   
                                delay (2000);
                                //delay(Tiempo_riego_2);
                                digitalWrite (33, RELAY_ON);     //Activa relé 4, que abre la ELECTROVALVULA 3
                                digitalWrite (27, RELAY_OFF);    //Desactiva relé 3, cierra la ELECTROVALVULA 2
                                delay (2000);
                                //delay(Tiempo_riego_3);
                                digitalWrite (25, RELAY_OFF);    //Desactiva relé 1, apaga la BOMBA
                                digitalWrite (33, RELAY_OFF);    //Desactiva relé 4, cierra la ELECTROVALVULA 3
                                riego1 = false; riego2 = false; riego3 = false;
                                regado1 = true; regado2 = true;regado3 = true;
                                regado_TODO =true;
                            }
                        }
                   }
               }
           if ((min_int > 45) && (regado1 == true) || (regado2 == true) || (regado3 == true)){
               riego1 = false;
               riego2 = false;
               riego3 = false;
               regado1 = false;
               regado2 = false;
               regado3 = false;
               regado_TODO = false ;
            }  
        }  
   delay(6000);     
}



//Funciones definidas por el usuario
//****************************************************************************************
String Fecha_Hora(){
  // El formato de hora es el siguiente:
  // 2018-05-28T16:00:13Z
  // Se requiere separar la fecha de la hora
  formattedDate = timeClient.getFormattedDate();
  
  // Serial.println(formattedDate);   //imprime la fecha y hora mostrando el formato para extraer los datos

  // Extract year/month/date ie reformat the NTP String
  int splitY = formattedDate.indexOf("-");
  String year = formattedDate.substring(0, splitY);
  //Serial.print("Año: "); Serial.println(year);
  
  int splitM = formattedDate.indexOf("-", splitY + 2 );
  String month = formattedDate.substring(5, splitM);
  //Serial.print("Mes: "); Serial.println(month);
  
  int splitD = formattedDate.indexOf("-", splitY + 3 );
  String mydate = formattedDate.substring(8, 10); //cheat!
  //Serial.print("Día: "); Serial.println(mydate);
  
  
  //Una vez que se tiene el mes, dia y año; se calcuala en numero de dia en el año
  Calculo_dia(month, mydate);   //nos devuelve Total_dias_anyo, FUNCION
 
  // Extract date and hour; muestra la fecha AAAA-MM-DD 
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("FECHA: ");
  Serial.println(dayStamp);
  
  // Extract time; y hora HH:MM:SS
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HORA: ");
  Serial.println(timeStamp);
 
  int splitH = formattedDate.indexOf(":");
  String hour = formattedDate.substring(11, 13);
  Serial.print("Horas: "); 
  Serial.println(hour);
  hr_int = hour.toInt(); //convertir el String "hour" a Entero "hr_int" 
  
  
  
  int splitMin = formattedDate.indexOf(":");
  String minu = formattedDate.substring(14, 16);
  Serial.print("Minutos: "); 
  Serial.println(minu);
  min_int = minu.toInt();                                            //convertir el String "minu" a Entero "min_int"
  //////////
  Serial.print("Minutos entero: ");
  Serial.println(min_int);
  ////////
  Serial.print("Dias transcurridos: ");
  Serial.println(Total_dias_anyo);

 
}



//********************************************************************************************************
float Toma_Temperatura(int hr_int, int min_int){
  //Registra la temperatura tomada por el DHT21 
     hum_21 = dht.readHumidity();
     temp_21 = dht.readTemperature();
     float f = dht.readTemperature(true);
     if(isnan(hum_21)||isnan(temp_21)||isnan(f)){
       Serial.println(F("Falla al leer el Sensor DHT"));
       return (0);
     }
     float hif = dht.computeHeatIndex(f, hum_21);
     float hic = dht.computeHeatIndex(temp_21, hum_21, false);
             //Serial.print(F("Humedad (%): ")); Serial.println(hum_21);
     Serial.print(F("Temperatura (°C): "));
     Serial.println(temp_21);
     return temp_21;
     
}


//************************************************************************************
//Lectura de humedad de suelo SECCION 1
bool Humedad_Sensor_1(){
   //EEPROM.begin(EEPROM_SIZE);
   humedad_1 = analogRead(SENSORPIN_1);
   Serial.println("Sensor 1:");
   Serial.println(humedad_1);
   if (humedad_1 > 2700){
    Serial.println("requiere riego la seccion 1...");
    return riego1 = true;
   }
   
 }
 
//************************************************************************************
//Lectura de humedad de suelo SECCION 2
bool Humedad_Sensor_2(){
   //EEPROM.begin(EEPROM_SIZE);
   humedad_2 = analogRead(SENSORPIN_2);
   Serial.println("Sensor 2:");
   Serial.println(humedad_2);
   if (humedad_2 > 2700){
    Serial.println("requiere riego la seccion 2...");
    return riego2 = true;
   }
 }
 
//************************************************************************************
//Lectura de humedad de suelo SECCION 3
bool Humedad_Sensor_3(){
   //EEPROM.begin(EEPROM_SIZE);
   humedad_3 = analogRead(SENSORPIN_3);
   Serial.println("Sensor 3:");
   Serial.println(humedad_3);
   if (humedad_3 > 2700){
    Serial.println("requiere riego la seccion 3...");
    return riego3 = true;
   }
 }




//********************************************************************************************
float Calculo_TempmMp(){
  //Leer lecturas de temperatura, calcular temperatura minima, maxima y promedio
  int aux_direccion = 0;
  //EEPROM.begin(EEPROM_SIZE);
  temp_min = EEPROM.get(aux_direccion, valor);
  temp_max = EEPROM.get(aux_direccion, valor);
  sum_temp = (EEPROM.get(aux_direccion, valor));
  for(int j = 1; j<24; j++){
   aux_direccion += sizeof(float);  
   float temp_auxiliar =  EEPROM.get(aux_direccion, valor);
   sum_temp = sum_temp + temp_auxiliar;
   if (temp_auxiliar < temp_min){
    temp_min = temp_auxiliar;
   }
   else {
    if (temp_auxiliar > temp_max){
      temp_max = temp_auxiliar;
    }
   }
  }
   
   Serial.println("Valores registrados de temperatura durante el día:");
   aux_direccion = 0;
   for(int j = 0; j<24; j++){
       
       Serial.print(EEPROM.get(aux_direccion, valor));
       Serial.print(" ");
       aux_direccion += sizeof(float); 
   }
   
   Serial.println();
   avg_temp = sum_temp /24;
   Serial.print("Temperatura mínima (°C): ");
   Serial.println(temp_min);
   Serial.print("Temperatura MAXIMA (°C): ");
   Serial.println(temp_max);
   Serial.print("Temperatura Promedio (°C): ");
   Serial.println(avg_temp);
   return temp_min;
   return temp_max;
   return avg_temp;
 }




//***********************************************************************************************
int Calculo_dia(String month, String mydate){
//calcula el numero de días transcurridos en el año a la fecha actual
//cada caso acumula los días de acuerdo al mes y finalmente se suman los dias transcurridos en el mes actual
    int numero_mes = month.toInt();
    switch (numero_mes){
           case 1:
                  numero_de_dias = 0;
                  break;
           case 2:
                  numero_de_dias = 31;
                  break;
           case 3:
                  numero_de_dias = 58;
                  //revisar si es año bisiesto
                  break;      
           case 4:
                  numero_de_dias = 90;
                  break;
           case 5:
                  numero_de_dias = 120;
                  break;
           case 6:
                  numero_de_dias = 151;
                  break;      
           case 7:
                  numero_de_dias = 181;
                  break;
           case 8:
                  numero_de_dias = 212;
                  break;
           case 9:
                  numero_de_dias = 243;
                  break;
           case 10:
                  numero_de_dias = 273;
                  break;
           case 11:
                  numero_de_dias = 304;
                  break;
           case 12:
                  numero_de_dias = 334;
                  break;
           return (numero_de_dias);
     }
     int numero_dia =  mydate.toInt();
     Total_dias_anyo = numero_de_dias + numero_dia;
     return (Total_dias_anyo);
}

//*************************************************************************************************
/*calculo de EVAPOTRANSPIRACION, ecuacion Penman-Monteith
  etc = kc*eto
  etc, evapotranspiracion del cultivo, medido en mm/día
  kc, coeficiente de cultivo, para pasto kc= 0.85
  eto, evaporacion del cultivo, mm/día

 
  eto = 0.0023 *(temp_media + 17.8)*(temp_max - temp_min)* 0.5 * ra   //dada en mm/dia
  
  la temperatura media se puede calcular facilmente como temp_media = (temp_max + temp_min)/2
  sin embargo se calculara por periodos de 24 hrs registrandose cada hora
 
   ra = ((24*60)/PI)* GSC * dr * (ws * sin(20.45)*sin(ds)+cos(20.45)*cos(ds)*sin(ws)))
*/

float Evapotranspiracion(int Total_dias_anyo, float temp_min, float temp_max, float avg_temp){
   //cÁLCULO DE "dr", distancia relativa inversa Tierra-sol
             /****convertir a radianes
                // dr = 1 + 0.033 * (cos(((2*PI)/365)* Total_dias_anyo));
                float aux_1 = (((((2*PI)/365)* Total_dias_anyo) * PI) / 180);
                    float dr = 1 + 0.033 * (cos(aux_1));
                          */
   dr = 1 + 0.033 * (cos(((2*PI)/365)* Total_dias_anyo));
                //  float aux_1 = (((((2*PI)/365)* Total_dias_anyo) * PI) / 180
   Serial.print("Distancia relativa inversa Tierra-Sol: ");
   Serial.print(dr, 5);
   Serial.println(" Unidades Astronómicas (1 AU = 1.496 x 108 Km)");


//CÁLCULO DE "ds"; declinacion solar
  /* ds = 0.409 *(sin((((2*PI)/365)* Total_dias_anyo)-1.39));
  //usar aux_1 que ya esta en radianes: sin(aux_1-1.39)
  // convertir a radianes
  float aux_2 = (((aux_1-1.39) * PI) / 180);
  
  float ds = 0.409 *(sin(aux_2));*/
   Serial.print(Total_dias_anyo);
   ds = 0.409 *(sin((((2*PI)/365)* Total_dias_anyo)-1.39));
  Serial.print("Declinación Solar: ");
  Serial.print(ds, 5);
  Serial.println(" radianes");
   
/*CÁLCULO DE "ws", angulo de radicacion a la puesta del sol
  la zona esta ubicada a 20° 27' 08" latitud norte
  equivalente a 20.45 grados decimales y 0.3570 radianes
*/
  x= 1 - ((tan(0.3570)*tan(0.3570))*(tan(ds)*tan(ds)));
 /* ws = (PI/2) - atan((-tan(0.3570)*tan(ds))/sqrt(x));
 //la tan() da radianes, debemos convertir a decimal
  float aux_4 = ((-tan(0.3570)*tan(ds))* (180 / PI));
  float aux_5 = (aux_4 / sqrt(x));
  float aux_6 = ((aux_5 * PI) / 180);
*/
  if (x <= 0){
    x = 0.00001;
    ws = (PI/2) - atan((-tan(0.3570)*tan(ds))/sqrt(x));
    Serial.print("Ángulo de Radiación a la puesta del Sol: ");
    Serial.print(ws, 5);
    Serial.println(" radianes");
  }
  else{
    //ws = (PI/2) - atan((-tan(0.3570)*tan(ds))/sqrt(x));
  ws = (PI/2) - atan((-tan(0.3570)*tan(ds))/sqrt(x));
  Serial.print("Ángulo de Radiación a la puesta del Sol: ");
  Serial.print(ws, 5);
  Serial.println(" radianes");
   }
  
  ra= ((24*60)/PI)* GSC * dr * ((ws * sin(20.45)*sin(ds))+(cos(20.45)*cos(ds)*sin(ws)));
  
  Serial.print("Radiación solar del día: ");
  Serial.print(ra, 5);
  Serial.println(" (MJ/m)/día");
  Serial.print("Temperaura MAXIMA: ");
  Serial.println(temp_max, 2);
  Serial.print("Temperaura mínima: ");
  Serial.println(temp_min, 2);
  Serial.print("Temperaura Promedio: ");
  Serial.println(avg_temp, 2);
  eto= 0.0023 *((avg_temp + 17.8)*(temp_max - temp_min)*0.5) *ra;
  Serial.print("Evapotranspiración inicial: ");
  Serial.print(eto, 5);
  Serial.println(" mm/d");
  etc = 0.85 * eto; 
  Serial.print("Evapotranspiración del pasto a recuperar : ");
  Serial.print(etc,5);
  Serial.println(" mm/d");
  return etc;
}

//*************************************************************
int Tiempo_de_Riego(float EFICIENCIA, float etc){
  //para poder calcular el tiempo de riego necesario para recuperar el
  //agua perdida por la planta a través de la EVAPOTRANSPIRACION, 
  //se calcula la Lamina de Riego Bruta (lrb)
  Serial.print("Eficiencia del Sistema de riego: ");
  Serial.println(EFICIENCIA, 2);
  Serial.print("Evapotranspiracion recibida: ");
  Serial.println(etc, 5);
  
  
  lrb = (etc / EFICIENCIA); // mm  de agua
  lrb_m = (lrb / 1000);           // m de agua    
  Serial.print("Lámina de Riego Requerida: ");
  Serial.print(lrb_m, 5);
  Serial.println(" m3 de agua");

  //con la lrb en metros, se puede obtene el volumen de agua por área, G
  //g = lbr_m  * area_de_riego      m3 de agua requeridos

  g_1 = (lrb_m * 94);       //area de la oficina
  g_2 = (lrb_m * 124);      //area de la casa
  g_3 = (lrb_m * 140);      //area del lago
  Serial.print("Cantidad de Agua, Sección 1: ");
  Serial.print(g_1, 5);
  Serial.println(" m3 de agua");
  Serial.print("Cantidad de Agua, Sección 2: ");
  Serial.print(g_2, 5);
  Serial.println(" m3 de agua");
  Serial.print("Cantidad de Agua, Sección 3: ");
  Serial.print(g_3, 5);
  Serial.println(" m3 de agua");
  
  //teniendo la cantidad de agua requerida en cada seccion se calcula el tiempo
  // de riego o de aplicación necesario, considerando el gasto del sistema de riego
  // Ta = G/Ga
  ga_1 = 0.09; //((10 * 9 ) / 1000);  //m3 por min
  ga_2 = 0.09; //((5 * 18) / 1000);
  ga_3 = 0.09; //((5 * 18) / 1000);
  Serial.println(ga_1, 5); 
  Serial.println(ga_2, 5);
  Serial.println(ga_3, 5);  
  ta_1 = (g_1 / ga_1);   //minutos
  ta_2 = (g_2 / ga_2);
  ta_3 = (g_3 / ga_3); 
  
  return ta_1;
  return ta_2;
  return ta_3;
}



//https://randomnerdtutorials.com/esp32-ntp-client-date-time-arduino-ide/
