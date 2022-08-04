/*
 * Este programa toma la Fecha y hora de acuerdo a la zona horaria de la CDMX, México
 * Registra las temperaturas amabientales dadas por el DHT21 en un perido de 24 horas
 * registradas cada hora.
 * Calcula la Temperatura mínima, Temperatura MAXIMA y Temperatura Promedio
 * Calcula la EVAPOTRANSPIRACION del pasto que será necesaria recuperar con el riego
 * 
 * Por: Cristina Sánchez Saldaña
 *      Rogelio Vázquez Nieves
 * Fecha: 14 de junio de 2022
*/

//Librerías
#include <WiFi.h>       // Habilita la conexión a la red (local e Internet) usando el Arduino WiFi shield. Con esta biblioteca puede crear instancias de servidores, clientes y enviar/recibir paquetes UDP a través de WiFi.
#include <NTPClient.h>  /* Crea un NTPClient para conectarse a un servidor de tiempo, permite obtener la fecha y la hora 
                          mediante el protocolo de tiempo de red (NTP); no necesita ningún hardware adicional.
                          Para ello, se requiere la biblioteca NTP Client bifurcada por Taranais.
                          en https://randomnerdtutorials-com.translate.goog/esp32-ntp-client-date-time-arduino-ide/?_x_tr_sl=en&_x_tr_tl=es&_x_tr_hl=es&_x_tr_pto=sc 
                          puedes descargar la librería y encontrar las instrucciones de instalación.*/
#include <WiFiUdp.h>    // Construye una instancia WiFiUDP de la clase WiFi UDP que puede enviar y recibir mensajes UDP
#include <DHT.h>
#include "math.h"       // define varias funciones matemáticas

/* Escriba sus credenciales de red en las siguientes variables, 
para que el ESP32 pueda establecer una conexión a Internet y obtener la fecha y la hora del servidor NTP.*/
const char* ssid     = "IZZI-8F7C"; //"GTYV";
const char* password = "C85261838F7C"; //"V1ncul4cI0n*#2021";

// Define NTP Client para obtener el tiempo, solicita fecha y hora de un servidor NTP.
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#define  DHTPIN  4           
#define  DHTTYPE DHT21 

//CONSTANTES
const float GSC = 0.082;                  //constante solar en  MJ/m  por min
const float EFICIENCIA = 0.80;             //eficiencia de riego de acuerdo al diseño por aspersores

// Variables to save date and time
String formattedDate;               //guarda la fecha en su formato original
String dayStamp;                    //guarda la fecha
String timeStamp;                   // guarda la hora
int numero_de_dias;                 //acumula los dias transcurridos por los meses completos
int numero_dia;                     // acumula los dias transcurridos del mes vigente
int Total_dias_anyo;                //almacena el total de días del año transcurridos a la fecha
float hum_21, temp_21, temp_min, temp_max, avg_temp, sum_temp;   //variables de humedad, temperatura (minima, maxima, promedio y suma de tmeeperaturas de 24 hrs)  
float etc, eto, kc, ra, x, dr, ws, ds;                           //variables para el calculo de la Evapotranspiración
float lrb, lrb_m, g, area_de_riego;                              // variables para el calculo de lamina de riego, gasto, area a reagar
float g_1, g_2, g_3, ga_1, ga_2, ga_3, ta_1, ta_2, ta_3;         //variables para el tiempo de riego


//Arreglos
float Temperaturas_DTH21[24];     //inicialmente se almacenaran las lecturas de temperaturas de un día en un arreglo
                                  //si se desenergiza la placa ESP32, se perderan

//Objects
DHT dht(DHTPIN, DHTTYPE);



void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);             //velocidad de salida al monitor serial
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {   //devuelve el estado de conexión
    delay(500);
    Serial.print(".");
  }
  // Muestra la dirección IP e inicia el servicor web
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

// Inicializa un NTPClient para obtener el tiempo
  timeClient.begin();
  // Establezca el tiempo de compensación en segundos para ajustar su zona horaria, por ejemplo
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(-17990); //zona horaria de CDMX

   Serial.println(F("DHTxx test!"));
   dht.begin();
 
}



void loop() {
  //Las siguientes líneas aseguran que obtengamos una fecha y hora válidas:
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  
  
  Fecha_Hora();  //Esta función muestra la Fecha y hora actual, llama a la función que calcula
                 // el numero de dias transcurridos y llama la función de registro de temperatura
                 // ambiental con periodos de 1 hora
  Calculo_TempmMp();  //Esta función calcula la temperatura minima, maxima y promedio
  Evapotranspiracion();  //Esta función calcula la evapotranspiracion
  Tiempo_de_Riego(etc);  //Calcula el tiempo de riego que requiere cada sección, recibiendo de entrada la evapotranspiración inicial
  
}


//Funciones definidas por el usuario
//****************************************************************************************
String Fecha_Hora(){
  // El formattedDate tiene el siguiente formato:
  // 2018-05-28T16:00:13Z
  // necesitamos separar la fecha del tiempo
  formattedDate = timeClient.getFormattedDate();
  
  // Serial.println(formattedDate);   //imprime la fecha y hora mostrando el formato para extraer los datos

  // Extrae  year/month/date del formato String dado por NTP 

  int splitY = formattedDate.indexOf("-");             //obtiene la primer cadena hasta encontrar el caracter indice "-"
  String year = formattedDate.substring(0, splitY);    //en year se almacena la subcadena a partir de la posicion 0
  //Serial.print("Año: "); Serial.println(year);
  
  int splitM = formattedDate.indexOf("-", splitY + 2 );   // en splitM, la siguiente cadena  
  String month = formattedDate.substring(5, splitM);      // en month se almacena la cadena obtenida a partir de la posicion 5, hasta encontrar el caracter de separacion "-"
  //Serial.print("Mes: "); Serial.println(month);
  
  int splitD = formattedDate.indexOf("-", splitY + 3 );  // en splitD la siguiente cadena, numero 3 
  String mydate = formattedDate.substring(8, 10);        // en mydate se almacena la cadena obtenida a partir de la posicion 8 a la 10
  
  //Serial.print("Día: "); Serial.println(mydate);
  
  int splitH = formattedDate.indexOf(":");               // obtiene la siguiente cadena a partir de T, delimitada po ":"
  String hour = formattedDate.substring(11, 13);         //alamacena en hour los caracteres que estan despues de la posicion 11 a la 13
  //Serial.print("Horas: "); Serial.println(hour);

  //Una vez que se tiene el mes, dia y año; se calcuala en numero de dia en el año
  Calculo_dia(month,mydate);   //nos devuelve Total_dias_anyo
 
  // Extraer la fecha; muestra la fecha AAAA-MM-DD 
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  Serial.print("FECHA: ");
  Serial.println(dayStamp);
  
  // Extrae la hora HH:MM:SS
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("HORA: ");
  Serial.println(timeStamp);

  Serial.print("Dias transcurridos: ");
  Serial.println(Total_dias_anyo);
  
  
  //Teniendo identificada la fecha, hora como información solamente y calculado el día del año
  //se registra la temperatura ambiental en la hora especificada
  for(int i = 0; i<24; i++){      //ciclo para hacer 24 registros en el arreglo
     int splitH = formattedDate.indexOf(":");
     String hour = formattedDate.substring(11, 13); //extrae la hora en que se registra la temperatura
                            //Serial.print("Horas: "); Serial.println(hour);
      Toma_Temperatura(hour);          //llama a la funcion de lectura de la temperatura ambiental a trves del DHT21
      Temperaturas_DTH21[i]= temp_21; //registra la lectura de la temperatura ambiental
      delay(3000);
  }
}

//********************************************************************************************
float Calculo_TempmMp(){
  //Leer lecturas de temperatura, calcular temperatura minima, maxima y promedio
  
  temp_min = Temperaturas_DTH21[0];     //obtiene le primer registro dentro del arreglo y la considera como la temperatura minima
  temp_max = Temperaturas_DTH21[0];     //obtiene le primer registro dentro del arreglo y la considera como la temperatura maxima
  for(int j = 1; j<24; j++){            // hace un ciclo de 24 para leer todos los datos contenidos en el arreglo
   float auxiliar = Temperaturas_DTH21[j];   //almacena en una variable auxiliar el valor obtenido de la posicion i
   if (auxiliar < temp_min){                 //compara el valor de la posicion i con la temepratura minima, registrada hasta ahora
    temp_min = auxiliar;                     // si auxiliar es menor que la temperatura minima, entonce auxiliar es ahora la temteperatura minima
   }
   else { // del if (auxiliar < temp_min)
    if (auxiliar > temp_max){                 //compara el valor de la posicion i con la temepratura maxima, registrada hasta ahora              
      temp_max = auxiliar;                    // si auxiliar es mayor que la temperatura maxima, entonce auxiliar es ahora la temteperatura maxima
    }
   }
  }
   sum_temp = 0;                              // inicializa valor              
   Serial.println("Valores registrados de temperatura durante el día:");
   for(int j = 0; j<24; j++){    // este ciclo for extrae los datos almacenados en el arreglo y los muestra
   Serial.print(Temperaturas_DTH21[j]);
   Serial.print(" ");
   sum_temp = sum_temp + Temperaturas_DTH21[j];   //va acumulando los datos del arreglo
   }
   
   Serial.println();
   avg_temp = sum_temp /24;                       // calcula el promedio de lo 24 datos de temepratura ambiental
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
    int numero_mes = month.toInt();      //convierte month de tipo String a tipo entero y lo almacena en numero mes
    switch (numero_mes){
           case 1:   //caso en que el mes actual sea enero
                  numero_de_dias = 0;   //aun no se ha cumplido un mes completo
                  break;
           case 2:    //caso en que el mes actual sea febrero
                  numero_de_dias = 31;   //indica el número de dias transcurridos en el mes anterior
                  break;
           case 3: //caso en que el mes actual sea marzo
                  numero_de_dias = 58; //indica el número de dias transcurridos en total de los meses enero y febrero
                  //revisar si es año bisiesto, queda pendiente
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
           return (numero_de_dias);   //devuelve la funcion, el total de dias transcurridos a la fecha actual, meses completos
     }
     int numero_dia =  mydate.toInt();   //convierte a entero el string que guarda el dia actual
     Total_dias_anyo = numero_de_dias + numero_dia;  //suma los dias del mes actual a los calculados anteriormente de los meses completos ya transcurridos
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
  
  lrb = etc / EFICIENCIA;         //calcula la lamina de riego bruta en mm/dia
  lrb_m = lrb / 1000;             // calcula la lamina de riego bruta en m        
  Serial.print("Lámina de Riego Requerida: ");
  Serial.print(lrb_m);
  Serial.println(" m de agua");

  //con la lrb en metros cubicos, se puede obtene el volumen de agua por planta, G
  //g = lbr_m * area_de_riego      el resultado seran m3 de agua requeridos

  g_1 = lrb_m * 94;       //area de la seccion 1
  g_2 = lrb_m * 124;      //area de la seccion 2
  g_3 = lrb_m * 140;      //area del seccion 3
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
  ga_1 = (10 * 9 ) / 1000;  //m3 por min, de acuerdo al calculo hidraulico de cada seccion
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


