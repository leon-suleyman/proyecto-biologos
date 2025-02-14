#define TINY_GSM_MODEM_SIM800 // Define the GSM modem model before including the library
//#include <TinyGsmClient.h>
//#include <SoftwareSerial.h>
#include <SoftwareSerial.h>



// Pin definitions for your SIM800 module
#define RX_PIN 10
#define TX_PIN 11
#define RESET_PIN 2
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

// Software serial for communication with the GSM module
/*SoftwareSerial SerialAT(RX_PIN, TX_PIN);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
*/
//Sim800L Sim800L(RX_PIN, TX_PIN);
bool error = false;
char* num_tel = "+541156628833";

String module_buffer;
SoftwareSerial SIM800L(RX_PIN, TX_PIN);
SoftwareSerial NANO_UNDER(RX_NANO_UNDER_PIN, TX_NANO_UNDER_PIN);
String _buffer;

String lecturas_nano_under = "";
String lecturas_nano_deeper = "";
int indice_lecturas_under = 0;
int indice_lecturas_deeper = 0;

enum : byte {IDLE, READ_UNDER, READ_DEEPER, SEND_SMS} estado = IDLE;


//setup al prenderse el dispositivo
void setup() {
  #if (SERIAL_DEBUG)
  Serial.begin(BAUD_RATE);
  Serial.println("Bienvenide al sistema de detección y comunicación");
  #endif

  pinMode(RESET_PIN, OUTPUT);

  SIM800L.begin(BAUD_RATE);

  NANO_UNDER.begin(BAUD_RATE);
  attachInterrupt(digitalPinToInterrupt(INTR_NANO_UNDER_PIN), interrupcionUnder, RISING);

  if (LED_FLAG) pinMode(LED_PIN, OUTPUT);

  //_buffer.reserve(BUFFER_RESERVE_MEMORY); // Reserve memory to prevent intern fragmention
}

void loop() {
  switch(estado){
    case IDLE:
      #if (SERIAL_DEBUG)
      serial_process();
      #endif
      if(indice_lecturas_under >= 12){
        estado = SEND_SMS;
      }
      break;
    case READ_UNDER:
      delay(100);
      //anulamos el buffer previo
      //_buffer[0] = NULL;
      //leo el buffer de la comunicación
      //strcat(_buffer, _readSerialUnder());
      _buffer = _readSerialUnder();
      //si me llegó algo
      if(_buffer != ""){
        //guardo los datos y le aviso que llegaron
        lecturas_nano_under += _buffer;
        //strcat(lecturas_nano_under, _buffer);
        indice_lecturas_under++;
        NANO_UNDER.print("llegó");
        //imprimo en pantalla si estamos en modo debug
        #if (SERIAL_DEBUG)
        Serial.print("\"");
        Serial.print(lecturas_nano_under);
        Serial.print("\"");
        Serial.println(indice_lecturas_under);
        #endif
      }else{
        #if (SERIAL_DEBUG)
        Serial.println("Error con la llegada de datos");
        #endif
      }
      estado = IDLE;
      break;

    case SEND_SMS:
      sendSms(num_tel, lecturas_nano_under);
      indice_lecturas_under = 0;
      lecturas_nano_under = "";
      estado = IDLE;
      break;
  }
}

//Interrupciones
void interrupcionUnder(){
  estado = READ_UNDER;
}

//lectura serial
String _readSerialUnder(){
  uint64_t timeOld = millis();

  while (!NANO_UNDER.available() && !(millis() > timeOld + TIME_OUT_READ_SERIAL))
  {
      delay(13);
  }

  String str = "";

  while(NANO_UNDER.available())
  {
      if (NANO_UNDER.available()>0)
      { 
          //int end_of_string = strlen(str);
          //str[end_of_string] = (char) NANO_UNDER.read();
          //str[end_of_string + 1] = NULL;
          str += (char) NANO_UNDER.read();
      }
  }

  return str;
}

String _readSerial_timeout(int timeout){
  uint64_t timeOld = millis();

  while (!SIM800L.available() && !(millis() > timeOld + timeout))
  {
      delay(13);
  }

  String str = "";

  while(SIM800L.available())
  {
      if (SIM800L.available()>0)
      {
          //int end_of_string = strlen(str);
          //str[end_of_string] = (char) NANO_UNDER.read();
          //str[end_of_string + 1] = NULL;
          str += (char) NANO_UNDER.read();
      }
  }

  return str;
}

//rutina para mandar un mensaje de texto
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
  //_buffer[0] = NULL;
  //strcat(_buffer, _readSerial_timeout(60000));
  _buffer = _readSerial_timeout(60000);
  
  #if SERIAL_DEBUG
  Serial.println(_buffer);
  #endif
  
  // Serial.println(_buffer);
  //expect CMGS:xxx   , where xxx is a number,for the sending sms.
  //if ((strstr(_buffer,"ER")) != NULL) {
  //    return true;
  //} else if ((strstr(_buffer,"CMGS")) != NULL) {
  //    return false;
  //} else {
  //  return true;
  //}
  if ((_buffer.indexOf("ER")) != -1) {
      return true;
  } else if ((_buffer.indexOf("CMGS")) != -1) {
      return false;
  } else {
    return true;
  }
  // Error found, return 1
  // Error NOT found, return 0
}

//funciones para testear y mandar comandos desde la computadora directamente al Arduino
#if SERIAL_DEBUG
String serial_buffer;
void serial_process(void) {  
  while (Serial.available() > 0) {
    char received = Serial.read();
    if (received == '\n') {
      if (!serial_parse()) Serial.println("ERR unknown/incorrect command"); 
      serial_buffer = String();
    }
    else {
      serial_buffer += received;
    }
  }
}

bool serial_parse(void) {
  error = false;
  String command, arg;
  int idx;
  if ((idx = serial_buffer.indexOf(' ')) != -1) {
    command = serial_buffer.substring(0, idx);
    arg = serial_buffer.substring(idx + 1);
  }
  else {
    command = serial_buffer;
    command.trim();
  }
  //control serial del Arduino, 'h' para el Hello y 'q' para mandar Bye.
  if (command.length() == 1) {
    if (command[0] == 'h'){
      Serial.println("mandando mensaje...");
      error = sendSms(num_tel, "Yo, World!");
      delay(5000);
        if (error) {
          Serial.println("Error al enviar mensaje :C");
        }else{
          Serial.println("mensaje enviado!");
        }
    } else {
      switch(command[0]) {
        case 'q':
          Serial.println("mandando mensaje...");
          error = sendSms(num_tel, "Hello World!");
          delay(5000);
          if (error) {
            Serial.println("Error al enviar mensaje :C");
          }else{
            Serial.println("mensaje enviado!");
          }
          break;
        case 'g':
          Serial.println("mandando mensaje...");
          error = sendSms(num_tel, "ring ring ring ring ring ring ring, Banana Phone!");
          delay(5000);
          if (error) {
            Serial.println("Error al enviar mensaje :C");
          }else{
            Serial.println("mensaje enviado!");
          }
          break;
        case 't':
          //_buffer[0] = NULL;
          //strcat(_buffer, _readSerialUnder());
          _buffer = _readSerialUnder();
          if(_buffer == ""){
            Serial.println("buffer vacío");
            break;
          }
          Serial.println(_buffer);
          break;
      }
    }
  }
  else{ 
    return false;
  }
  return true;
}
#endif

