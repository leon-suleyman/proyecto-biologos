// Programa incluye lectura 3x fotodiodos (inferior: fluorimetro para clorofila + turbidez; superior: irradiancia solar) y 1x DS18B20 para T°.
// Modulo RTC DS1302, modulo tarjeta SD, LED azul (fluorescencia), LED rojo (encendido).
// Modulo ADS1115 y OpAmp CA3140.
// Modulo Reed Switch.
// Amplitud lectura para fluorescencia GAIN_FOUR.
// Instalar modo Sleep para colecta de datos cada hora.

// pines:
// D2: Dallas DS18b20
// D3: interrupción para el nano con la SIM800L
// D4: SDA  
// D5: SCL   I2c para 2 chips: ADS1115: Addr= 0x48  y tambien RTC: DS3231: ADDR= 0x68
// D6: Led Fluor
// D7: RX comunicación con el nano con la SIM800L
// D8: reedswitch
// D9: TX comunicación con el nano con la SIM800L
// D10 , 11, 12, 13 es la SPI: CS, MOSI, MOSO, SCK
//
//AD0 es el BPW34 fluor
//AD1 es el BPW34 Irradiancia
//AD2 es el BPW34 turbidez (OBS: no hay conexión para este en la placa que armamos con Leo)


#include <SPI.h>              // Include SPI library (needed for the SD card)
#include <SD.h>               // Include SD library
#include <OneWire.h>               // Incluir promagra OneWire lectura
#include <DallasTemperature.h>   // Incluir programa DallaTemperature lectura
#include <Adafruit_ADS1X15.h>  // incluye libreria conversor ADS1115
#include <Wire.h>   // incluye libreria para lector temperatura DS18B20
#include "SD.h"    //incluye libreria para modulo tarjeta SD
#include <RTClib.h>   // incluye libreria para el manejo del modulo RTC DS3231
#include <SPI.h>
#include <SoftwareSerial.h>


const int oneWirePin = 2; //sensor dallas

OneWire oneWireBus(oneWirePin);
DallasTemperature sensor(&oneWireBus);


#define SSpin 10  // Pin 10, a CS/SS (Chip/Slave Select)



const int pinDatosDQ = 2;                         // Pin donde se conecta el bus l-wire
const int pinLed = 6; //led fluo
const int pinIntrpt = 3;
const int pin_rx_sim = 7;
const int pin_tx_sim = 9;

SoftwareSerial nano_sim(pin_rx_sim, pin_tx_sim);


Adafruit_ADS1115 ads;
const float multiplier = 0.1875F;

File logFile;

// RTC_DS1307 rtc;
RTC_DS3231 rtc;

String daysOfTheWeek[7] = { "Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado" };
String monthsNames[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo",  "Junio", "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre" };

DateTime tiempos_de_lecturas[13];
int lect_fluoro[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int lect_irradiancia[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int lect_temperatura[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int indice_lectura_sensores = 0;


void setup()
{
  {
    Serial.begin(9600);
    Serial.print(F("Iniciando SD ..."));
    if (!SD.begin(10))
    {
      Serial.println(F("Error al iniciar"));
      return;
    }
    Serial.println(F("Iniciado correctamente"));

    sensor.begin(); //inicia el sensor de temp dallas
    // seteo del AD (ads1115)
    // Desconentar el correcto pra este caso (4 fluorescencia y luego sera 1 en irradiancia)
    // ads.setGain(GAIN_TWOTHIRDS);  +/- 6.144V  1 bit = 0.1875mV (default)
    // ads.setGain(GAIN_ONE);        +/- 4.096V  1 bit = 0.125mV
    // ads.setGain(GAIN_TWO);        +/- 2.048V  1 bit = 0.0625mV
    ads.setGain(GAIN_FOUR);    //   +/- 1.024V  1 bit = 0.03125mV
    // ads.setGain(GAIN_EIGHT);      +/- 0.512V  1 bit = 0.015625mV
    // ads.setGain(GAIN_SIXTEEN);    +/- 0.256V  1 bit = 0.0078125mV
  
  
    ads.begin(); //ads1115

    nano_sim.begin(9600);
    attachInterrupt(digitalPinToInterrupt(pinIntrpt), reportarAlNanoSim, RISING);

    pinMode(pinLed, OUTPUT); // pin LED en output fluorom

    digitalWrite(6, LOW);
 
  }

  //dde aca inicia el RTC:
  {
    if (!rtc.begin()) {
      Serial.println(F("No encuentro al RTC: Verificar conexiones y bateria"));
      while (1);
    }

    // Si se ha perdido la corriente, fijar fecha y hora
    if (rtc.lostPower())
    {
      // Fijar a fecha y hora de compilacion
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    
      // Fijar a fecha y hora específica. En el ejemplo, 21 de Enero de 2016 a las 03:00:00
      // rtc.adjust(DateTime(2016, 1, 21, 3, 0, 0));
    }
  }
}   //termina el setup


void loop()
{
    digitalWrite(6, HIGH);
    //delay(120000); //2 min para el led
 
  // Obtener fecha actual del ds3231 y mostrar por Serial
    DateTime now = rtc.now();
  //printDate(now);
 
  // lee Fluorescencia
    lect_fluoro[indice_lectura_sensores] = readSensorFluoro();   //readSensorFluoro()
    //int var = readSensorFluoro();

  // lee irradiancia
    lect_irradiancia[indice_lectura_sensores] = readSensorIrradiancia();
    //int var2 = readSensorIrradiancia();

  // lee temperatura:
    lect_temperatura[indice_lectura_sensores] = readSensorTemperatura();
    //int Temp = readSensorTemperatura();

  //avanza indice de sensores
    //indice_lectura_sensores++;

  // Abrir archivo y escribir valor - FTIR es: Fluorescencia, Temperatura, Irradiancia, Registro
    //File logFile = SD.open("DATOS.txt", FILE_WRITE);
 //
    //if (logFile) {
   //
    //  now = rtc.now();
//
    //  logValue(logFile, now, val, val2, Temp);
    //  logFile.close();
    ////apaga led
    //  digitalWrite(6, LOW);
   //
//
    //}else {
    //  Serial.println(F("Error al abrir el archivo"));
    //}
//
    //logFile = SD.open("DATOS.txt", FILE_READ);
    //if (logFile){
    //  String data_txt = readFile(logFile);
    //  Serial.println(data_txt);
//
    //  logFile.close();
    //}else{
    //  Serial.println(F("Error al abrir el archivo"));
    //}

    now = rtc.now();

    //logValue("DATOS.txt", now, val, val2, Temp);

    //String data_txt = readFile("DATOS.txt");
    String data_txt = "";
    for(int i = 0; i < indice_lectura_sensores; i++){
      data_txt = data_txt + stringLecturaSensores(i);
    }
    Serial.println(data_txt);

    if(indice_lectura_sensores > 13){
      indice_lectura_sensores = 0;
    }

    delay(10000); // 60 segundos (TIEMPO de delay LOOP)
}
//termina el PP

//desde aca Funciones declaracion:

String stringLecturaSensores(int indice){
  String lectura = "";
  DateTime tiempo = tiempos_de_lecturas[indice];
  lectura = lectura + String(tiempo.year()) + "/" + String(tiempo.month()) + "/" + String(tiempo.day()) + " " + String(tiempo.hour()) + ":" + String(tiempo.minute()) + ":" + String(tiempo.second()) + ";" ;
  lectura = lectura + String(lect_fluoro[indice]) + ";" + String(lect_irradiancia[indice]) + ";" + String(lect_temperatura[indice]) + ";"; 
  return lectura;
}

void reportarAlNanoSim(){

  nano_sim.println("Hello World!");

}

// Funcion sensores
int readSensorIrradiancia()
  {
  int i;
  int sval = 0;
  ads.setGain(GAIN_ONE);      //  +/- 4.096V  1 bit = 0.125mV
  ads.begin();
  //int16_t adc0;
  // int16_t adc0, adc1;

 
  for (i = 0; i < 5; i++){
    sval = ads.readADC_SingleEnded(1);   // sensor on analog pin 0
    delay(100);
  }

  sval = sval / 5;    //promedio de las 5 medidas tomadas cada 100 ms

  Serial.print("AIN1: "); Serial.println(sval);  //solo imprime por serie el 1 er valor - es para testeo unicamente -
  Serial.println(" ");
  delay(10);
  return sval;
}


int readSensorTemperatura()
{
  int TempDallas = 0;
  sensor.requestTemperatures();
  Serial.print("Temperatura en sensor 0: ");
  TempDallas = sensor.getTempCByIndex(0);
  Serial.print(sensor.getTempCByIndex(0));
  Serial.println(" ºC");
  return TempDallas;
}


int readSensorFluoro()
// el led debe estar prendido 2 min antes
{
  ads.setGain(GAIN_FOUR);    //   +/- 1.024V  1 bit = 0.03125mV
  ads.begin();
  int16_t adc0;
  // int16_t adc0, adc1;
  int i;
  int sval0 = 0;
  ads.setGain(GAIN_ONE);      //  +/- 4.096V  1 bit = 0.125mV
 
  //int16_t adc0;
  // int16_t adc0, adc1;

 
  for (i = 0; i < 5; i++){
    sval0 = ads.readADC_SingleEnded(1);   // sensor on analog pin 0
    delay(100);
  }

  sval0 = sval0 / 5;    //promedio de las 5 medidas tomadas cada 100 ms

  Serial.print("AIN0: "); Serial.println(sval0);  //solo imprime por serie el valor - es para testeo unicamente -
  Serial.println(" ");
  delay(10);
  return sval0;
 
}

void logValue(String filepath, DateTime date, int value, int value2, int value3)
{
  File fs = SD.open(filepath, FILE_WRITE);
  if(fs){
    fs.print(date.year(), DEC);
    fs.print('/');
    fs.print(date.month(), DEC);
    fs.print('/');
    fs.print(date.day(), DEC);
    fs.print(" ");
    fs.print(date.hour(), DEC);
    fs.print(':');
    fs.print(date.minute(), DEC);
    fs.print(':');
    fs.print(date.second(), DEC);
    fs.print(" ;");
    fs.print(value);
    fs.print('; ');
    fs.print(value2);
    fs.print('; ');
    fs.println(value3);

    fs.close();
  }else{
    Serial.println("Error al abrir el archivo");
  }

 
 
}

String readFile(String filepath){
  String file_txt = "";
  File fs = SD.open(filepath, FILE_READ);
  if(fs){
    while(fs.peek() != -1){
      char data = fs.read();
      file_txt.concat(data);
    }

    fs.close();
  }else{
    Serial.println("Error al abrir el archivo");
  }
  return file_txt;
}
