#define TINY_GSM_MODEM_SIM800 // Define the GSM modem model before including the library
//#include <TinyGsmClient.h>
//#include <SoftwareSerial.h>
#include <Sim800L.h>

// Your GPRS credentials
const char apn[] = "igprs.claro.com.ar"; // Replace with your APN
const char user[] = ""; // Leave empty if not needed
const char pass[] = ""; // Leave empty if not needed

// Server details
const char server[] = "eu.httpbin.org";
const int port = 80;

// Pin definitions for your SIM800 module
#define RX_PIN 7
#define TX_PIN 8
#define BAUD_RATE 9600
#define SERIAL_DEBUG 0 // poner en 1 para controlar por terminal serial de arduino


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
Sim800L Sim800L(RX_PIN, TX_PIN);

//setup al prenderse el dispositivo
void setup() {
  #if (SERIAL_DEBUG)
  Serial.begin(BAUD_RATE);
  Serial.println("Bienvenide al sistema de detección y comunicación");
  #endif

  Sim800L.begin(BAUD_RATE);
  Sim800L.sendSms("+451156628833", "Hello World!");
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
    if (command[0] >= '1' && command[0] <= '5') set_speed_junta(command[0] - '1', 1);
    else if (command[0] == 'h') Sim800L.sendSms("+451156628833", "Hello World!");
    else {
      switch(command[0]) {
        case 'q': Sim800L.sendSms("+451156628833", "Bye World!"); break;
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
