#define TINY_GSM_MODEM_SIM800 // Define the GSM modem model before including the library
//#include <TinyGsmClient.h>
//#include <SoftwareSerial.h>
#include <SoftwareSerial.h>



// Pin definitions for your SIM800 module
#define RX_PIN 10
#define TX_PIN 11
#define BAUD_RATE 9600
#define LED_FLAG	true 	// true: use led.	 false: don't user led.
#define LED_PIN 	13 		// pin to indicate states.
#define BUFFER_RESERVE_MEMORY	350
#define TIME_OUT_READ_SERIAL	5000

//Pins para comunicarse con el nano a menor distancia
#define RX_NANO_UNDER_PIN 7
#define TX_NANO_UNDER_PIN 6
#define INTR_NANO_UNDER_PIN 3

//Pins para comunicarse con el nano a mayor distnacia
#define RX_NANO_DEEPER_PIN 5
#define TX_NANO_DEEPER_PIN 4
#define INTR_NANO_DEEPER_PIN 2

#define SERIAL_DEBUG 1 // poner en 1 para recibir mensajes por terminal serial de arduino



#if SERIAL_DEBUG
#define SERIAL_DBG(x) x
#else
#define SERIAL_DBG(x)
#endif

bool error = false;
//número de teléfono al que le llegaran los mensajes del SIM800L
char* num_tel = "+541156628833";

//comunicaciones por SoftwareSerial a los componentes
SoftwareSerial SIM800L(RX_PIN, TX_PIN);
SoftwareSerial NANO_UNDER(RX_NANO_UNDER_PIN, TX_NANO_UNDER_PIN);
SoftwareSerial NANO_DEEPER(RX_NANO_DEEPER_PIN, TX_NANO_DEEPER_PIN);
//String _buffer;
char _buffer[40];

//buffer donde guardamos las lecturas que llegan de los Nanos sumergidos
//char lecturas_nanos_sumergidos[912];
char lecturas_nano_under[469];
char lecturas_nano_deeper[469];

//cuantas lecturas tenemos guardadas
//uint8_t indice_lecturas = 0;
uint8_t indice_lecturas_deeper = 0;
uint8_t indice_lecturas_under = 0;

//Los estados del Arduino Nano que va a la superficie conectado al SIM800L
enum : byte {IDLE, UNDER_READ, DEEPER_READ, SEND_SMS} estado = IDLE;
/*
  * IDLE : duerme esperando que le manden instrucciones de anotar datos, si hay 12 datos anotados de alguno de los demas Arduinos Nano, cambia a SEND_SMS_correspondiente
  * UNDER_READ : lee la comunicación con el Arduino nano sumergido más cercano y anota la lectura que le dió. De haber sido la 12ava lectura. 
  * DEEPER_READ : mismo que UNDER_READ pero comunicandose con el Arduino Nano sumergido más lejano. 
  * SEND_SMS_[...] : manda un mensaje con las 12 lecturas dirarias del Arduino Nano [...].
*/

//flag que nos dice si mientras estabamos leyendo los datos de un Arduino, otro nos manda request de prestarle atención. 
bool request_lectura_paralela = false;


//setup al prenderse el dispositivo
void setup() {
  #if (SERIAL_DEBUG)
  Serial.begin(BAUD_RATE);
  Serial.println("Bienvenide al sistema de detección y comunicación");
  #endif

  //iniciamos la comunicación con el SIM800L
  SIM800L.begin(BAUD_RATE);

  //iniciamos la comunicación con el arduino nano sumergido más cercano, Nano Under
  NANO_UNDER.begin(BAUD_RATE);
  //le asociamos una rutina de interrupción al Pin de interrupción conectado al Nano Under
  attachInterrupt(digitalPinToInterrupt(INTR_NANO_UNDER_PIN), interrupcionUnder, RISING);
  //iniciamos comunicación con el Nano sumergido más lejano, Nano Deeper
  NANO_DEEPER.begin(BAUD_RATE);
  //le asociamos una rutina de interrupción al Pin de interrupción conectado al Nano Deeper
  attachInterrupt(digitalPinToInterrupt(INTR_NANO_DEEPER_PIN), interrupcionDeeper, RISING);

  //ponemos la SIM800L en modo bajo consumo
  //putSIM800LToLowPower();

  if (LED_FLAG) pinMode(LED_PIN, OUTPUT);
}

void loop() {
  //maquina de estados: depende el estado, la rutina del loop es diferente
  switch(estado){
    //si estamos en IDLE, no hace nada, pero si nota que tenemos 24 lecturas guardadas, se pone para mandar el mensaje
    case IDLE:
      //if(indice_lecturas >= 24){
      if(indice_lecturas_under == 12 || indice_lecturas_deeper == 12){
        //ponemos la SIM800L en consumo normal para mandar mensajes
        //putSIM800LToNormal();
        SIM800L.listen();
        estado = SEND_SMS;
      }
    
    break;

    //si hay que leer del Nano Under
    case UNDER_READ:
      delay(100);
      //anulamos el buffer previo
      _buffer[0] = NULL;
      //leo el buffer de la comunicación
      _readSerialUnder();
      //si me llegó algo
      if(_buffer[0] != NULL){
        //guardo los datos
        strcat(lecturas_nano_under, _buffer);
        indice_lecturas_under++;
        //imprimo en pantalla si estamos en modo debug
        #if (SERIAL_DEBUG)
        Serial.print(lecturas_nano_under);
        Serial.print(lecturas_nano_deeper);
        Serial.println(indice_lecturas_under);
        #endif
      }else{
        #if (SERIAL_DEBUG)
        Serial.println("Error con la llegada de datos del under");
        #endif
      }
      //vuelvo a IDLE
      estado = IDLE;
      //pero si hay otra lectura esperando
      if(request_lectura_paralela){
        #if (SERIAL_DEBUG)
        Serial.println("hubo lectura paralela al leer el under");
        #endif
        //cambio a hacer la lectura del Nano Deeper
        NANO_DEEPER.listen();
        estado = DEEPER_READ;
        request_lectura_paralela = false;
      }
    break;
    
    //si hay que leer del Nano Deeper
    case DEEPER_READ:
      delay(100);
      //anulamos el buffer previo
      _buffer[0] = NULL;
      //leo el buffer de la comunicación
      _readSerialDeeper();
      //si me llegó algo
      if(_buffer[0] != NULL){
        //guardo los datos
        strcat(lecturas_nano_deeper, _buffer);
        indice_lecturas_deeper++;
        //imprimo en pantalla si estamos en modo debug
        #if (SERIAL_DEBUG)
        Serial.print(lecturas_nano_under);
        Serial.print(lecturas_nano_deeper);
        Serial.println(indice_lecturas_deeper);
        #endif
      }else{
        #if (SERIAL_DEBUG)
        Serial.println("Error con la llegada de datos del deeper");
        #endif
      }
      //vuelvo a IDLE
      estado = IDLE;
      //pero si hay otra lectura esperando
      if(request_lectura_paralela){
        #if (SERIAL_DEBUG)
        Serial.println("hubo lectura paralela al leer el deeper");
        #endif
        //cambio a lectura del Nano Under
        NANO_UNDER.listen();
        estado = UNDER_READ;
        request_lectura_paralela = false;
      }

    break;
    
    //si tenemos que mandar por SMS las lecturas guardadas
    case SEND_SMS:
      #if (SERIAL_DEBUG)
      Serial.println("Por enviar datos por SMS");
      #endif
      //si está lleno el buffer del nano uner, mandamos ese
      if(indice_lecturas_under >= 12){
        sendLongSms(num_tel, lecturas_nano_under);
        lecturas_nano_under[0] = NULL;
        indice_lecturas_under = 0;
      }
      //si está lleno el buffer del nano deeper, mandamos ese
      if(indice_lecturas_deeper >= 12){
        sendLongSms(num_tel, lecturas_nano_deeper);
        lecturas_nano_deeper[0] = NULL;
        indice_lecturas_deeper = 0;
      }
      #if (SERIAL_DEBUG)
      Serial.println("Datos enviados por SMS");
      #endif
      estado = IDLE;
      //si hubo lectura paralela, mandamos a leer al Nano Under pero sin sacar el flag pq no sabemos cual es, y si no lee nada, igual pasamos a leer al Deeper y ya.
      if(request_lectura_paralela){
        #if (SERIAL_DEBUG)
        Serial.println("hubo lectura mientras mandabamos mensaje");
        #endif
        //cambio a lectura del Nano Under
        NANO_UNDER.listen();
        estado = UNDER_READ;
      }
    break;
    
  }
  //delay(100);
}

//Interrupciones
void interrupcionUnder(){
  if(estado == IDLE){
    //la función listen() nos deja utilizar este puerto para recibir información
    NANO_UNDER.listen();
    estado = UNDER_READ;
  //si estamos en medio de una lectura o de mandar mensaje, encolamos la lectura
  }else if(estado != UNDER_READ){
    request_lectura_paralela = true;
  }
}
void interrupcionDeeper(){
  if(estado == IDLE){
    //la función listen() nos deja utilizar este puerto para recibir información
    NANO_DEEPER.listen();
    estado = DEEPER_READ;
  }else if(estado != DEEPER_READ){
    request_lectura_paralela = true;
  }
}

//lectura serial

void _readSerialUnder(){
  //tomamos medida de tiempo como referencia
  uint64_t timeOld = millis();

  //esperamos 5 segundos a que llegue la comunicación
  while (!NANO_UNDER.available() && !(millis() > timeOld + TIME_OUT_READ_SERIAL))
  {
      delay(13);
  }

  _buffer[0] = NULL;
  char temp[2];
  //cuando llegó la copiamos al _buffer
  while(NANO_UNDER.available())
  {
      if (NANO_UNDER.available()>0)
      { 
        temp[1] = NULL;
        temp[0] = (char) NANO_UNDER.read();
        strcat(_buffer, temp);
      }
  }
}

void _readSerialDeeper(){
  uint64_t timeOld = millis();

  while (!NANO_DEEPER.available() && !(millis() > timeOld + TIME_OUT_READ_SERIAL))
  {
      delay(13);
  }

  _buffer[0] = NULL;
  char temp[2];

  while(NANO_DEEPER.available())
  {
      if (NANO_DEEPER.available()>0)
      { 
        temp[1] = NULL;
        temp[0] = (char) NANO_DEEPER.read();
        strcat(_buffer, temp);
      }
  }
}
//lo mismo que las anteriores pero esta ademas toma parametro de cuanto tiempo esperar: timeout
void _readSerialSim(int timeout){
  uint64_t timeOld = millis();

  while (!SIM800L.available() && !(millis() > timeOld + timeout))
  {
      delay(13);
  }

  _buffer[0] = NULL;
  char temp[2];

  while(SIM800L.available())
  {
      if (SIM800L.available()>0)
      { 
        temp[1] = NULL;
        temp[0] = (char) SIM800L.read();
        strcat(_buffer, temp);
      }
  }
}

//rutina para mandar un mensaje de texto
void sendLongSms(char* num, char* message){
  //nuestro de buffer de envios tiene el máximo que podemos mandar por sms, 160 caracteres.
  char buffer_envios[160];
  buffer_envios[0] = NULL;
  char datos_de_lectura[40];
  datos_de_lectura[0] = NULL;
  char caracter;
  int i = 0;
  int cant_chars = strlen(message);
  //indice para el caracter NULL en datos_de_lectura
  int indice_fin_string = 0;
  while(message[i] != NULL && i < cant_chars){
    //agrego el proximo caracter
    char caracter = message[i];
    datos_de_lectura[indice_fin_string + 1] = NULL;
    datos_de_lectura[indice_fin_string] = caracter;
    indice_fin_string += 1;

    //si es un end of line, tenemos una lectura completa en datos_de_lectura y podemos agregarla al mensaje
    if(caracter == '\n'){
      //si el mensaje se excede al agregar, entonces lo enviamos  y despues lo agregamos
      if(strlen(datos_de_lectura) < 160 && strlen(datos_de_lectura) + strlen(buffer_envios) >= 160){
        #if (SERIAL_DEBUG)
          Serial.print("SMS saliente : ");
          Serial.println(buffer_envios);
        #endif
        sendSms(num, buffer_envios);
        buffer_envios[0] = NULL;
      }
      strcat(buffer_envios, datos_de_lectura);
      //strcat(buffer_envios, "\n");
      datos_de_lectura[0] = NULL;
      indice_fin_string = 0;
    }

    i++;
  }
  #if (SERIAL_DEBUG)
    Serial.print("SMS saliente : ");
    Serial.println(buffer_envios);
  #endif
  sendSms(num, buffer_envios);
}

bool sendSms( String num, String msg){
  SIM800L.println("\r\n"); //limpiar antes de mandar cosas
  SIM800L.println ("AT+CMGF=1"); 	//set sms to text mode
  delay(100);

  SIM800L.println ("AT+CMGS=\"" + num + "\"");  	// command to send sms
  delay(100);
  
  SIM800L.print (msg);
  delay(100);
  
  SIM800L.write(26);
  delay(2000);
  _buffer[0] = NULL;
  _readSerialSim(60000);
  
  #if SERIAL_DEBUG
  Serial.println(_buffer);
  #endif

  //expect CMGS:xxx   , where xxx is a number,for the sending sms.
  if ((strstr(_buffer,"ER")) != NULL) {
      return true;
  } else if ((strstr(_buffer,"CMGS")) != NULL) {
      return false;
  } else {
    return true;
  }
  // Error found, return 1
  // Error NOT found, return 0
}

bool putSim800LToLowPower(){
  SIM800L.println("\r\n"); //limpiar antes de mandar cosas
  SIM800L.println("AT+CFUN=0");
  delay(100);
}

bool putSim800LToNormal(){
  SIM800L.println("\r\n"); //limpiar antes de mandar cosas
  SIM800L.println("AT+CFUN=1");
  delay(100);
}

bool sendSms( String num, char* msg){
  SIM800L.println("\r\n"); //limpiar antes de mandar cosas
  SIM800L.println ("AT+CMGF=1"); 	//set sms to text mode
  delay(100);

  SIM800L.println ("AT+CMGS=\"" + num + "\"");  	// command to send sms
  delay(100);
  
  SIM800L.print (msg);
  delay(100);
  
  SIM800L.write(26);
  delay(2000);
  _buffer[0] = NULL;
  _readSerialSim(60000);
  
  #if SERIAL_DEBUG
  Serial.println(_buffer);
  #endif
  
  //expect CMGS:xxx   , where xxx is a number,for the sending sms.
  if ((strstr(_buffer,"ER")) != NULL) {
      return true;
  } else if ((strstr(_buffer,"CMGS")) != NULL) {
      return false;
  } else {
    return true;
  }
  // Error found, return 1
  // Error NOT found, return 0
}
