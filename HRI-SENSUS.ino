#include <common.h>
//3.2.1
#include <FirebaseESP8266.h>
#include <FirebaseFS.h>
#include <Utils.h>

//1.0.4
#include <ESPDateTime.h>
//#include <DateTime.h>
//#include <TimeElapsed.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
//2.0.3alpha
#include <WiFiManager.h>
#include <string.h>

/********DEFINICION DE ZONA HORARIA********/
#define TZ_America_Santiago "<-04>4<-03>,M9.1.6/24,M4.1.6/24"

/********DEFINICION DE LOS PINES********/
#define PIN_MED D2      //Definicion del PIN MEDIDOR


/*********DEFINICION DEL HOST DE FirebaseArduino********/
#define FIREBASE_HOST "firebaseHOST"
#define FIREBASE_AUTH "FIREBASEAUTH"


/******** DEFINICION DE VARIABLES CONSTANSTES Y GLOBALES********/
  const float litrosPorPulso = 1; // La cantidad de litros por pulso, según la especificación del HRI

  volatile int pulseConter;
  float consumo;
  float consumoPrevio;
  String basedatos;
  bool estado = false;
  String fechaEvento;

  // Meses
  const char* months[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", 
                              "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};
  
  FirebaseData fbdo;
 

/********** CABECERAS DE FUNCIONES*************/

  String obtenerMACid(void);
  void confirmacionSet(int);
  void ICACHE_RAM_ATTR ISRCountPulse(void);
  void setupDateTime(void);
  bool verificarReinicioRemoto(void);
  bool verificarInicializacion(void);
  void reinicioTotal(void);
  void reinicioParcial(void);
  
/********** SETUP DEL ARDUINO (AL INICIAR)*************/
void setup() {
  Serial.begin(9600);
  
  pinMode(PIN_MED,INPUT);       //Inicialización del pin medidor
  pinMode(LED_BUILTIN, OUTPUT); //Inicialización del pin led
  attachInterrupt(digitalPinToInterrupt(PIN_MED),ISRCountPulse,RISING);        // DEFINICION DE LA INTERRUPCIONo

  //SI ESTA SIN PODER CONECTARSE A WIFI- NECESIDAD DE CONFIGURAR
  WiFiManager wifiManager;
  
  //CONECTARSE A WIFI AUTOMATICAMENTE
  wifiManager.autoConnect("AquaxisDevice","configuraciondispositivo");

  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect, we should reset as see if it connects");
    wifiManager.resetSettings();
  }
  Serial.println("Ya estás conectado");
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); //AQUAXIS
  setupDateTime();

  //Test funcion obtener MAC
  String auxiliar = obtenerMACid();
  Serial.print("Esta fue la MAC-ID obtenida del codigo: ");
  Serial.println(auxiliar);
  Serial.println("Se inicializaron los streams de RealtimeDB");
  
  //EN CASO DE TENER INFORMACION PREVIA EN LA BASE DE DATOS (CONSUMO), AGARRARLA.
  basedatos = "dispositivos/"+obtenerMACid();

  if(verificarInicializacion()){
    Serial.println("Se leerá el consumo de origen");
    Firebase.getFloat(fbdo,basedatos+"/consumo");
    consumo = fbdo.floatData();
  }
}

/********** FUNCION LOOP (CODIGO QUE SE EJECUTA DE MANERA CONSTANTE *************/
void loop() {

  consumo+= contadorPulsos * litrosPorPulso;
  // Imprime el consumo de agua en tiempo real
  Serial.print("Consumo de agua: ");
  Serial.print(consumoLitros);
  Serial.println(" litros");

  if(consumo>consumoPrevio){
    timeEvent = DateTime.getTime();
    fechaEvento = DateTime.toUTCString();
    
    estado = true;
    consumoAnterior = consumo;
    digitalWrite(LED_BUILTIN, HIGH); // Enciende el LED
    delay(500); // Espera 500 ms
    digitalWrite(LED_BUILTIN, LOW);      
    Firebase.setBool(fbdo,basedatos+"/estado",estado);
    Firebase.setFloat(fbdo,basedatos+"/consumo",consumo);
  }
  else{
    Serial.println("El dispositivo se apago");
    estado = false;
    Firebase.setBool(fbdo,basedatos+"/estado",estado);
  }
  
  delay(1000);
}

/********** FUNCIONES UTILIZADAS EN EL CODIGO *************/

void confirmacionSet(int tipo){
    switch(tipo){
      case 1:
        Serial.println("El consumo ha sido establecido en cero");
        break;
      case 2:
        Serial.println("El caudal ha sido establecido en cero");
        break;
    }
    return;
}

void ICACHE_RAM_ATTR ISRCountPulse(){
  pulseConter++;
}

//Obtiene la MAC (ID unico del dispositivo arduino
String obtenerMACid(){
    byte mac[6];
    String macString;
    WiFi.macAddress(mac);
    for (byte i = 0; i < 6; ++i)
    {
      char buf[3];
      sprintf(buf, "%02X", mac[i]);
      macString += buf;
    }
    return macString;
}

//Funcion que pone el tiempo

void setupDateTime() {
  // setup this after wifi connected
  // you can use custom timeZone,server and timeout
  DateTime.setTimeZone(TZ_America_Santiago);
  DateTime.setServer("ntp.shoa.cl");
  // this method config ntp and wait for time sync
  // default timeout is 10 seconds
  DateTime.begin();
  if (!DateTime.isTimeValid()) {
    Serial.println("Failed to get time from server.");
  }
}


bool verificarInicializacion(){
  Firebase.getBool(fbdo,"registeredDevices/"+obtenerMACid());
  bool flag = fbdo.boolData();
  if(flag){
    return true;
  }
  consumo = 0;
  consumoPrevio = 0;
  return false;
}
