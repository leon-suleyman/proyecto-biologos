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

#define SERIAL_DEBUG 1 // poner en 1 para controlar por terminal serial de arduino



#if SERIAL_DEBUG
#define SERIAL_DBG(x) x
#else
#define SERIAL_DBG(x)
#endif

bool error = false;
char* num_tel = "+541156628833";

SoftwareSerial SIM800L(RX_PIN, TX_PIN);
SoftwareSerial NANO_UNDER(RX_NANO_UNDER_PIN, TX_NANO_UNDER_PIN);
SoftwareSerial NANO_DEEPER(RX_NANO_DEEPER_PIN, TX_NANO_DEEPER_PIN);
//String _buffer;
char _buffer[40];

char lecturas_nanos_sumergidos[912];
uint8_t indice_lecturas_under = 0;
uint8_t indice_lecturas_deeper = 0;
uint8_t indice_lecturas = 0;

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

  SIM800L.begin(BAUD_RATE);

  NANO_UNDER.begin(BAUD_RATE);
  attachInterrupt(digitalPinToInterrupt(INTR_NANO_UNDER_PIN), interrupcionUnder, RISING);
  NANO_DEEPER.begin(BAUD_RATE);
  attachInterrupt(digitalPinToInterrupt(INTR_NANO_DEEPER_PIN), interrupcionDeeper, RISING);

  if (LED_FLAG) pinMode(LED_PIN, OUTPUT);

  //_buffer.reserve(BUFFER_RESERVE_MEMORY); // Reserve memory to prevent intern fragmention
}

void loop() {
  switch(estado){
    case IDLE:
      if(indice_lecturas >= 24){
        //estado = SEND_SMS;
      }
    break;

    case UNDER_READ:
      delay(100);
      //anulamos el buffer previo
      _buffer[0] = NULL;
      //leo el buffer de la comunicación
      _readSerial(NANO_UNDER, TIME_OUT_READ_SERIAL);
      //si me llegó algo
      if(_buffer[0] != NULL){
        //guardo los datos y le aviso que llegaron
        strcat(lecturas_nanos_sumergidos, _buffer);
        indice_lecturas_under++;
        NANO_UNDER.print("llegó");
        //imprimo en pantalla si estamos en modo debug
        #if (SERIAL_DEBUG)
        Serial.print(lecturas_nanos_sumergidos);
        #endif
      }else{
        #if (SERIAL_DEBUG)
        Serial.println("Error con la llegada de datos del under");
        #endif
      }
      //talvez agregar mutex?
      estado = IDLE;
      if(request_lectura_paralela){
        #if (SERIAL_DEBUG)
        Serial.println("hubo lectura paralela al leer el under");
        #endif
        estado = DEEPER_READ;
        request_lectura_paralela = false;
      }
    break;
    
    case DEEPER_READ:
      delay(100);
      //anulamos el buffer previo
      _buffer[0] = NULL;
      //leo el buffer de la comunicación
      //_readSerial(NANO_DEEPER).toCharArray(_buffer, sizeof(_buffer));
      //strcat(_buffer, _readSerial(NANO_DEEPER));
      _readSerial(NANO_DEEPER, TIME_OUT_READ_SERIAL);
      //si me llegó algo
      if(_buffer[0] != NULL){
        //guardo los datos y le aviso que llegaron
        strcat(lecturas_nanos_sumergidos, _buffer);
        indice_lecturas_deeper++;
        NANO_DEEPER.print("llegó");
        //imprimo en pantalla si estamos en modo debug
        #if (SERIAL_DEBUG)
        Serial.print(lecturas_nanos_sumergidos);
        #endif
      }else{
        #if (SERIAL_DEBUG)
        Serial.println("Error con la llegada de datos del deeper");
        #endif
      }

      estado = IDLE;
      if(request_lectura_paralela){
        #if (SERIAL_DEBUG)
        Serial.println("hubo lectura paralela al leer el deeper");
        #endif
        estado = UNDER_READ;
        request_lectura_paralela = false;
      }

    break;
    

    case SEND_SMS:
      #if (SERIAL_DEBUG)
      Serial.println("Por enviar datos por SMS");
      #endif
      sendLongSms(num_tel, lecturas_nanos_sumergidos);
      #if (SERIAL_DEBUG)
      Serial.println("Datos enviados por SMS");
      #endif
      lecturas_nanos_sumergidos[0] = NULL;
      indice_lecturas_under = 0;
      estado = IDLE;
    break;
    
  }
}

//Interrupciones
//agregar mutex ?
void interrupcionUnder(){
  if(estado == IDLE){
    estado = UNDER_READ;
  }else if(estado != UNDER_READ){
    request_lectura_paralela = true;
  }
}
void interrupcionDeeper(){
  if(estado == IDLE){
    estado = DEEPER_READ;
  }else if(estado != DEEPER_READ){
    request_lectura_paralela = true;
  }
}

//lectura serial
//quiero probar hacer una función sola de serial y simplemente pasarle como parametro el SoftwareSerial

//char* _readSerial(SoftwareSerial sw){
void _readSerial(SoftwareSerial sw, int timeout){
  uint64_t timeOld = millis();

  while (!sw.available() && !(millis() > timeOld + timeout))
  {
      delay(13);
  }

  _buffer[0] = NULL;

  while(sw.available())
  {
      if (sw.available()>0)
      { 
        char temp[2];
        temp[1] = NULL;
        temp[0] = (char) sw.read();
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
        //sendSms(num, buffer_envios);
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
  //sendSms(num, buffer_envios);
}

bool sendSms( String num, String msg){
  SIM800L.println("\r\n"); //limpiar antes de mandar cosas
  SIM800L.println ("AT+CMGF=1"); 	//set sms to text mode
  delay(100);
  //_buffer=_readSerial();

  SIM800L.println ("AT+CMGS=\"" + num + "\"");  	// command to send sms
  //SIM800L.print (num);
  //SIM800L.println("\"");
  delay(100);
  //_buffer=_readSerial();
  
  SIM800L.print (msg);
  //SIM800L.print ("\r");
  delay(100);
  //_buffer=_readSerial();
  
  SIM800L.write(26);
  delay(2000);
  _buffer[0] = NULL;
  //strcat(_buffer, _readSerial_timeout(60000));
  //_readSerial_timeout(60000).toCharArray(_buffer, sizeof(_buffer));
  //_buffer = _readSerial_timeout(60000);
  _readSerial(SIM800L, 60000);
  
  #if SERIAL_DEBUG
  Serial.println(_buffer);
  #endif
  
  // Serial.println(_buffer);
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
  //_readSerial_timeout(60000).toCharArray(_buffer, sizeof(_buffer));
  _readSerial(SIM800L, 60000);
  
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
