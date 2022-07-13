/*
 * Este programa toma la Fecah y hora de acuerdo a la zona horaria de la CDMX, México
 * Registra las temperaturas amabientales dadas por el DHT21 en un perido de 24 horas
 * registradas cada hora.
 * Calcula la Temperatura minima, Temperatura MAXIMA y Temperatura Promedio
 * Calcula la EVAPOTRANSPIRACION del pasto que será necesaria recuperar con el riego
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

// Replace with your network credentials
const char* ssid     = "IZZI-8F7C"; //"GTYV";
const char* password = "C85261838F7C"; //"V1ncul4cI0n*#2021";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#define  DHTPIN  4           
#define  DHTTYPE DHT21 

//CONSTANTES
const float GSC = 0.082;                  //constante solar en  MJ/m  por min
const float EFICIENCIA = 0.80;             //eficiencia de riego de acuerdo al diseño por aspersores

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;
int numero_de_dias;  //acumula los dias transcurridos por los meses completos
int numero_dia;      // acumula los dias transcurridos del mes vigente
int Total_dias_anyo;
float hum_21, temp_21, temp_min, temp_max, avg_temp, sum_temp;  
float etc, eto, kc, ra, x, dr, ws, ds;    //variables de la Evapotranspiración
float lrb, lrb_m, g, area_de_riego; 
float g_1, g_2, g_3, ga_1, ga_2, ga_3, ta_1, ta_2, ta_3;    //variables para el tiempo de riego


//Arreglos
float Temperaturas_DTH21[24];

//Objects
DHT dht(DHTPIN, DHTTYPE);



void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
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
 
}



void loop() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  Fecha_Hora();  //Esta función muestra la Fecha y hora actual, llama a la función que calcula
                 // el numero de dias transcurridos y llama la función de registro de temperatura
                 // ambiental con periodos de 1 hora
  Calculo_TempmMp();  //Esta función calcula la temperatura minima, maxima y promedio
  Evapotranspiracion();
  Tiempo_de_Riego(etc);
  
}


//Funciones definidas por el usuario
//****************************************************************************************
String Fecha_Hora(){
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
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
  
  int splitH = formattedDate.indexOf(":");
  String hour = formattedDate.substring(11, 13);
  //Serial.print("Horas: "); Serial.println(hour);

  //Una vez que se tiene el mes, dia y año; se calcuala en numero de dia en el año
  Calculo_dia(month,mydate);   //nos devuelve Total_dias_anyo
 
  // Extract date and hour; muestra la fecha AAAA-MM-DD 
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("FECHA: ");
  Serial.println(dayStamp);
  
  // Extract time; y hora HH:MM:SS
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HORA: ");
  Serial.println(timeStamp);

  Serial.print("Dias transcurridos: ");
  Serial.println(Total_dias_anyo);
  
  
  //Teniendo identificada la fecha, hora como información solamente y calculado el día del año
  //se registra la temperatura ambiental en la hora especificada
  for(int i = 0; i<24; i++){
     int splitH = formattedDate.indexOf(":");
     String hour = formattedDate.substring(11, 13);
                            //Serial.print("Horas: "); Serial.println(hour);
      Toma_Temperatura(hour);
      Temperaturas_DTH21[i]= temp_21; //registra la lectura de la temperatura ambiental
      delay(3000);
  }
}

//********************************************************************************************
float Calculo_TempmMp(){
  //Leer lecturas de temperatura, calcular temperatura minima, maxima y promedio
  
  temp_min = Temperaturas_DTH21[0];
  temp_max = Temperaturas_DTH21[0];
  for(int j = 1; j<24; j++){
   float auxiliar = Temperaturas_DTH21[j];
   if (auxiliar < temp_min){
    temp_min = auxiliar;
   }
   else {
    if (auxiliar > temp_max){
      temp_max = auxiliar;
    }
   }
  }
   sum_temp = 0;
   Serial.println("Valores registrados de temperatura durante el día:");
   for(int j = 0; j<24; j++){
   Serial.print(Temperaturas_DTH21[j]);
   Serial.print(" ");
   sum_temp = sum_temp + Temperaturas_DTH21[j];
   }
   
   Serial.println();
   avg_temp = sum_temp /24;
   Serial.print("Temperatura mínima (°C): ");
   Serial.println(temp_min);
   Serial.print("Temperatura MAXIMA (°C): ");
   Serial.println(temp_max);
   Serial.print("Temperatura Promedio (°C): ");
   Serial.println(avg_temp);
}


//********************************************************************************************************
float Toma_Temperatura(String hour){
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
     Serial.print("Horas: ");
     Serial.println(hour);
           //Serial.print(F("Humedad (%): ")); Serial.println(hum_21);
     Serial.print(F("Temperatura (°C): "));
     Serial.println(temp_21);
     
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

float Evapotranspiracion(){
   //cÁLCULO DE "dr", distancia relativa inversa Tierra-sol

   float dr = 1 + 0.033 * (cos(((2*PI)/365)* Total_dias_anyo));
   Serial.print("Distancia relativa inversa Tierra-Sol: ");
   Serial.println(dr);


//CÁLCULO DE "ds"; declinacion solar
  float ds = 0.409 *(sin((((2*PI)/365)* Total_dias_anyo)-1.39));
  Serial.print("Declinación Solar: ");
  Serial.print(ds);
  Serial.println(" radianes");
   
/*CÁLCULO DE "ws", angulo de radicacion a la puesta del sol
  la zona esta ubicada a 20° 27' 08" latitud norte
  equivalente a 20.45 grados decimales y 0.3570 radianes
*/
  
  x= 1- ((tan(0.3570)*tan(0.3570))*(tan(ds)*tan(ds)));
  if (x <= 0){
    x = 0.00001;
    ws = (PI/2) - atan((-tan(0.3570)*tan(ds))/sqrt(x));
    Serial.print("Ángulo de Radiación a la puesta del Sol: ");
    Serial.print(ws);
    Serial.println(" radianes");
  }
  else{
  ws = (PI/2) - atan((-tan(0.3570)*tan(ds))/sqrt(x));
  Serial.print("Ángulo de Radiación a la puesta del Sol: ");
  Serial.print(ws);
  Serial.println(" radianes");
   }
  
  //ra= ((24*60)/PI)* GSC * dr * ((ws * sin(20.45)*sin(ds))+(cos(20.45)*cos(ds)*sin(ws)));
  ra= ((24*60)/PI)* GSC * dr * ((ws * sin(0.3570)*sin(ds))+(cos(0.3570)*cos(ds)*sin(ws)));
  Serial.print("Radiación solar del día: ");
  Serial.print(ra);
  Serial.println(" (MJ/m)/día");
  eto= 0.0023 *(avg_temp +17.8)*(temp_max - temp_min)*0.5 *ra;
  Serial.print("evapotranspiración inicial: ");
  Serial.print(eto);
  Serial.println(" mm/d");
  etc = 0.85 * eto; 
  Serial.print("Evapotranspiración del pasto a recuperar : ");
  Serial.print(etc);
  Serial.println(" mm/d");
}

//*************************************************************
int Tiempo_de_Riego(float etc){
  //para poder calcular el tiempo de riego necesario para recuperar el
  //agua perdida por la planta a través de la EVAPOTRANSPIRACION, 
  //se calcula la Lamina de Riego Bruta (lrb)
  
  lrb = etc / EFICIENCIA * 100;
  lrb_m = lrb / 1000;               
  Serial.print("Lámina de Riego Requerida: ");
  Serial.print(lrb_m);
  Serial.println(" m de agua");

  //con la lrb en metros, se puede obtene el volumen de agua por planta, G
  //g = lbr_m * area_de_riego      m3 de agua requeridos

  g_1 = lrb_m * 94;       //area de la oficina
  g_2 = lrb_m * 124;      //area de la casa
  g_3 = lrb_m * 140;      //area del lago
  Serial.print("Cantidad de Agua, Sección 1: ");
  Serial.print(g_1);
  Serial.println(" m3 de agua");
  Serial.print("Cantidad de Agua, Sección 2: ");
  Serial.print(g_2);
  Serial.println(" m3 de agua");
  Serial.print("Cantidad de Agua, Sección 3: ");
  Serial.print(g_3);
  Serial.println(" m3 de agua");
  
  //teniendo la cantidad de agua requerida en cada seccion se calcula el tiempo
  // de riego o de aplicación necesario, considerando el gasto del sistema de riego
  // Ta = G/Ga
  ga_1 = (10 * 9 ) / 1000;  //m3 por min
  ga_2 = (5 * 18) / 1000;
  ga_3 = (5 * 18) / 1000;
  Serial.println(ga_1); 
  ta_1 = g_1 / ga_1;   //minutos
  ta_2 = g_2 / ga_2;
  ta_3 = g_3 / ga_3; 
  Serial.print("Tiempo de riego, Sección 1: ");
  Serial.print(ta_1);
  Serial.println(" minutos");
  Serial.print("Tiempo de riego, Sección 2: ");
  Serial.print(ta_2);
  Serial.println(" minutos");
  Serial.print("Tiempo de riego, Sección 3: ");
  Serial.print(ta_3);
  Serial.println(" minutos");
}



//https://randomnerdtutorials.com/esp32-ntp-client-date-time-arduino-ide/
