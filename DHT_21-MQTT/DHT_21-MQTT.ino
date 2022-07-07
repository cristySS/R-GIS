/*
 * conexion sensor DHT21 con ESP32 a MQTT
 * Por: Cristina Sánchez Saldaña
 *      Rogelio Vázquez Nieves
 * Fecha: 14 de junio de 2022
 * 
 * Este programa envia los valores de temperatura del sensor DHT21
 * por MQTT.
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
#include <WiFi.h>           // Biblioteca para el control de WiFi
#include <PubSubClient.h>   //Biblioteca para conexion MQTT
#include "DHT.h"            //libreria a utilizar DHT sensor library


// Constantes
#define DHTPIN 4          // a que pin se conectara el DHT21 al ESP32
#define DHTTYPE DHT21      //tipo de sensor que se estara usando DHT21 (AM2301)

//Datos de WiFi
const char* ssid = "*********";       // Aquí debes poner el nombre de tu red
const char* password = "**********";  // Aquí debes poner la contraseña de tu red

//Datos del broker MQTT --->EN CONSOLA nslookup broker.hivemq.com
const char* mqtt_server = "127.0.0.1";    // Si estas en una red local, coloca la IP asignada, en caso contrario, coloca la IP publica
IPAddress server(127,0,0,1);


// Definición de objetos
WiFiClient espClient;             // Este objeto maneja los datos de conexion WiFi
PubSubClient client(espClient);   // Este objeto maneja los datos de conexion al broker
DHT dht(DHTPIN, DHTTYPE);         //se crea el objeto a usar del sensor


// Variables
long timeNow, timeLast;   // Variables de control de tiempo no bloqueante
int wait = 5000;          // Indica la espera cada 5 segundos para envío de mensajes MQTT
float hum_21;             //almacena el valor de la humedad leida por el sensor DHT21
float temp_21;            //almacena el valor de la temperatura leida por el sensor DHT21


// Inicialización del programa
// Condiciones iniciales - Se ejecuta sólo una vez al energizar
void setup() {                           // Inicio de void setup ()
  // Iniciar comunicación serial
  Serial.begin (115200);

  Serial.println();
  Serial.println();
  Serial.print("Conectar a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // Esta es la función que realiz la conexión a WiFi
 
  while (WiFi.status() != WL_CONNECTED) { // Este bucle espera a que se realice la conexión
    delay(500); //dado que es de suma importancia esperar a la conexión, debe usarse espera bloqueante
    Serial.print(".");  // Indicador de progreso
    delay (5);
  }

  // Cuando se haya logrado la conexión, el programa avanzará, por lo tanto, puede informarse lo siguiente
  Serial.println();
  Serial.println("WiFi conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
  
  //Serial.println(F("DHTxx test!"));      //mensaje de prueba de conectividad con el sensor

  //delay (1000); // Esta espera es solo una formalidad antes de iniciar la comunicación con el broker

  // Conexión con el broker MQTT
  client.setServer(server, 1883); // Conectarse a la IP del broker en el puerto indicado
  client.setCallback(callback); // Activar función de CallBack, permite recibir mensajes MQTT y ejecutar funciones a partir de ellos
  delay(1500);  // Esta espera es preventiva, espera a la conexión para no perder información

  dht.begin();                           //iniciaciacion del objeto  
  
  timeLast = millis (); // Inicia el control de tiempo
  
  
}// Fin de void setup

// Cuerpo del programa - Se ejecuta constantemente
void loop() {// Inicio de void loop
  //Verificar siempre que haya conexión al broker
  if (!client.connected()) {
    reconnect();  // En caso de que no haya conexión, ejecutar la función de reconexión, definida despues del void setup ()
  }// fin del if (!client.connected())
  client.loop(); // Esta función es muy importante, ejecuta de manera no bloqueante las funciones necesarias para la comunicación con el broker

  timeNow = millis(); // Control de tiempo para esperas no bloqueantes
  if (timeNow - timeLast > wait) { // Manda un mensaje por MQTT cada cinco segundos
    timeLast = timeNow; // Actualización de seguimiento de tiempo
 
  //lectura de humedad o temperatura, solo requerimos temperatura ambiente
    temp_21 = dht.readTemperature(); //lectura de temperatura

   
  //revision si existe falla en la lectura del sensor
    if (isnan(temp_21)) {
       Serial.println(F("Falla al leer el sensor DHT"));
       return;
     } // fin de if

   
    char dataString[8]; // Define una arreglo de caracteres para enviarlos por MQTT, especifica la longitud del mensaje en 8 caracteres
    dtostrf(temp_21, 1, 2, dataString);  // Esta es una función nativa de leguaje AVR que convierte un arreglo de caracteres en una variable String
    Serial.print("Temperatura: "); // Se imprime en monitor solo para poder visualizar que el evento sucede
    Serial.println(dataString);
    client.publish("esp32/temp", dataString); // Esta es la función que envía los datos por MQTT, especifica el tema y el valor
  }// fin del if (timeNow - timeLast > wait)

}// Fin de void loop

// Funciones del usuario

// Esta función permite tomar acciones en caso de que se reciba un mensaje correspondiente a un tema al cual se hará una suscripción
void callback(char* topic, byte* message, unsigned int length) {

  // Indicar por serial que llegó un mensaje
  Serial.print("Llegó un mensaje en el tema: ");
  Serial.print(topic);

  // Concatenar los mensajes recibidos para conformarlos como una varialbe String
  String messageTemp; // Se declara la variable en la cual se generará el mensaje completo  
  for (int i = 0; i < length; i++) {  // Se imprime y concatena el mensaje
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  // Se comprueba que el mensaje se haya concatenado correctamente
  Serial.println();
  Serial.print ("Mensaje concatenado en una sola variable: ");
  Serial.println (messageTemp);

  // En esta parte puedes agregar las funciones que requieras para actuar segun lo necesites al recibir un mensaje MQTT

  // Ejemplo, en caso de recibir el mensaje true - false, se cambiará el estado del led soldado en la placa.
  // El ESP323CAM está suscrito al tema esp/output
  /*if (String(topic) == "esp32/output") {  // En caso de recibirse mensaje en el tema esp32/output
    if(messageTemp == "true"){
      Serial.println("Led encendido");
      digitalWrite(flashLedPin, HIGH);
    }// fin del if (String(topic) == "esp32/output")
    else if(messageTemp == "false"){
      Serial.println("Led apagado");
      digitalWrite(flashLedPin, LOW);
    }// fin del else if(messageTemp == "false")
  }// fin del if (String(topic) == "esp32/output")
  */
}// fin del void callback

// Función para reconectarse
void reconnect() {
  // Bucle hasta lograr conexión
  while (!client.connected()) { // Pregunta si hay conexión
    Serial.print("Tratando de contectarse...");
    // Intentar reconexión
    if (client.connect("ESP32CAMClient")) { //Pregunta por el resultado del intento de conexión
      Serial.println("Conectado");
      client.subscribe("ESP32/temp"); // Esta función realiza la suscripción al tema
    }// fin del  if (client.connect("ESP32CAMClient"))
    else {  //en caso de que la conexión no se logre
      Serial.print("Conexion fallida, Error rc=");
      Serial.print(client.state()); // Muestra el codigo de error
      Serial.println(" Volviendo a intentar en 5 segundos");
      // Espera de 5 segundos bloqueante
      delay(5000);
      Serial.println (client.connected ()); // Muestra estatus de conexión
    }// fin del else
  }// fin del bucle while (!client.connected())
}// fin de void reconnect()
