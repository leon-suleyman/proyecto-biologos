#define TINY_GSM_MODEM_SIM800 // Define the GSM modem model before including the library
#include <TinyGsmClient.h>
#include <SoftwareSerial.h>

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

// Software serial for communication with the GSM module
SoftwareSerial SerialAT(RX_PIN, TX_PIN);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

void setup() {}

void loop() {
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
}
