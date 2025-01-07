#define TINY_GSM_MODEM_SIM800 // Define the GSM modem model before including the library
//#include <TinyGsmClient.h>
//#include <SoftwareSerial.h>
#include <Sim800L.h>
#include <SoftwareSerial.h>



// Pin definitions for your SIM800 module
#define RX_PIN 10
#define TX_PIN 11
#define RESET_PIN 2
#define BAUD_RATE 9600
#define LED_FLAG	true 	// true: use led.	 false: don't user led.
#define LED_PIN 	13 		// pin to indicate states.
#define BUFFER_RESERVE_MEMORY	255
#define TIME_OUT_READ_SERIAL	5000

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
String module_buffer;
SoftwareSerial sw_serial(RX_PIN, TX_PIN);
String _buffer;

String _readSerial(){
  uint64_t timeOld = millis();

  while (!sw_serial.available() && !(millis() > timeOld + TIME_OUT_READ_SERIAL))
  {
      delay(13);
  }

  String str;

  while(sw_serial.available())
  {
      if (sw_serial.available()>0)
      {
          str += (char) sw_serial.read();
      }
  }

  return str;
}

String _readSerial_timeout(int timeout){
  uint64_t timeOld = millis();

  while (!sw_serial.available() && !(millis() > timeOld + timeout))
  {
      delay(13);
  }

  String str;

  while(sw_serial.available())
  {
      if (sw_serial.available()>0)
      {
          str += (char) sw_serial.read();
      }
  }

  return str;
}

bool sendSms( String num, String msg){
  sw_serial.println ("AT+CMGF=1"); 	//set sms to text mode
  _buffer=_readSerial();
  sw_serial.println ("AT+CMGS=\"" + num + "\"");  	// command to send sms
  //sw_serial.print (num);
  //sw_serial.println("\"");
  _buffer=_readSerial();
  sw_serial.print (msg);
  //sw_serial.print ("\r");
  _buffer=_readSerial();
  sw_serial.write(26);
  _buffer=_readSerial_timeout(60000);
  // Serial.println(_buffer);
  //expect CMGS:xxx   , where xxx is a number,for the sending sms.
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

//setup al prenderse el dispositivo
void setup() {
  #if (SERIAL_DEBUG)
  Serial.begin(BAUD_RATE);
  Serial.println("Bienvenide al sistema de detección y comunicación");
  #endif

  pinMode(RESET_PIN, OUTPUT);

  sw_serial.begin(BAUD_RATE);

  if (LED_FLAG) pinMode(LED_PIN, OUTPUT);

  _buffer.reserve(BUFFER_RESERVE_MEMORY); // Reserve memory to prevent intern fragmention

  sendSms("+541156628833", "Hello World!");
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
      error = sendSms("+541156628833", "Yo, World!");
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
          error = sendSms("+541156628833", "Hello World!");
          delay(5000);
          if (error) {
            Serial.println("Error al enviar mensaje :C");
          }else{
            Serial.println("mensaje enviado!");
          }
          break;
        case 't':
          Serial.println("mandando mensaje...");
          error = sendSms("+541156628833", "1234567890");
          delay(5000);
          if (error) {
            Serial.println("Error al enviar mensaje :C");
          }else{
            Serial.println("mensaje enviado!");
          }
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

void loop() {
  #if (SERIAL_DEBUG)
  serial_process();
  #endif
/*
// Start communication with the computer
Serial.begin(9600);
delay(10);

// Set up the GSM module serial communication
SerialAT.begin(BAUD_RATE);
delay(3000); // Give time for the GSM module to initialize
Serial.println("Initializing modem...");
modem.restart();

// Unlock your SIM card with a PIN if needed
modem.simUnlock("1234");
delay(15000);
Serial.print("Connecting to ");
Serial.print(apn);
if (!modem.gprsConnect(apn, user, pass)) {
Serial.println(" fail");
while (true);
}
Serial.println(" success");

// Connect to the server
if (!client.connect(server, port)) {
Serial.println("Connection to server failed");
while (true); // Terminate after failure
}
Serial.println("Connected to server");

// Make an HTTP GET request
client.print(String("GET /get HTTP/1.1\r\n") + "Host: " + server + "\r\n" + "Connection: close\r\n\r\n");

// Wait for server to respond
unsigned long timeout = millis();
while (client.connected() && millis() - timeout < 10000L) {
while (client.available()) {
char c = client.read();
Serial.print(c);
timeout = millis();
}
}

// Close the connection
client.stop();
Serial.println("Server disconnected");
// Terminate the program
while (true);
*/
}
