// Programa incluye lectura 3x fotodiodos (inferior: fluorimetro para clorofila + turbidez; superior: irradiancia solar) y 1x DS18B20 para T°.
// Modulo RTC DS1302, modulo tarjeta SD, LED azul (fluorescencia), LED rojo (encendido).
// Modulo ADS1115 y OpAmp CA3140.
// Modulo Reed Switch.
// Amplitud lectura para fluorescencia GAIN_FOUR.
// Instalar modo Sleep para colecta de datos cada hora.

// pines:
// D8: reedswitch
// D6: Led Fluor
// D2: Dallas DS18b20
// D4: SDA  
// D5: SCL   I2c para 2 chips: ADS1115: Addr= 0x48  y tambien RTC: DS3231: ADDR= 0x68
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


const int oneWirePin = 2; //sensor dallas

OneWire oneWireBus(oneWirePin);
DallasTemperature sensor(&oneWireBus);


#define SSpin 10  // Pin 10, a CS/SS (Chip/Slave Select)



const int pinDatosDQ = 2;                         // Pin donde se conecta el bus l-wire
const int pinLed = 6; //led fluo




Adafruit_ADS1115 ads;
const float multiplier = 0.1875F;

int val; //valor fluore
int val2; // valor irradiancia
int Temp; // del dallas

File logFile;

// RTC_DS1307 rtc;
RTC_DS3231 rtc;

String daysOfTheWeek[7] = { "Domingo", "Lunes", "Martes", "Miercoles", "Jueves", "Viernes", "Sabado" };
String monthsNames[12] = { "Enero", "Febrero", "Marzo", "Abril", "Mayo",  "Junio", "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre" };


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
  pinMode(6, OUTPUT); // pin LED en output fluorom
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
  delay(120000); //2 min para el led
 
  // Obtener fecha actual del ds3231 y mostrar por Serial
  DateTime now = rtc.now();
  //printDate(now);
 
  // lee Fluorescencia
  val = readSensorFluoro();   //readSensorFluoro()

  // lee irradiancia
  val2 = readSensorIrradiancia();

  // lee temperatura:
  Temp = readSensorTemperatura();

  // Abrir archivo y escribir valor - FTIR es: Fluorescencia, Temperatura, Irradiancia, Registro
  logFile = SD.open("FTIR.txt", FILE_WRITE);
 
  if (logFile) {
   
    DateTime now = rtc.now();

    logValue(now, val, val2, Temp);
    logFile.close();

    //apaga led
    digitalWrite(6, LOW);
   

  }
  else {
    Serial.println(F("Error al abrir el archivo"));
  }
  delay(6000); // 60 segundos (TIEMPO de delay LOOP)
}
//termina el PP

//desde aca Funciones declaracion:

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
  int TempDallas;
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

void logValue(DateTime date, int value, int value2, int value3)
{
  logFile.print(date.year(), DEC);
  logFile.print('/');
  logFile.print(date.month(), DEC);
  logFile.print('/');
  logFile.print(date.day(), DEC);
  logFile.print(" ");
  logFile.print(date.hour(), DEC);
  logFile.print(':');
  logFile.print(date.minute(), DEC);
  logFile.print(':');
  logFile.print(date.second(), DEC);
  logFile.print(" ;");
  logFile.print(value);
  logFile.print('; ');
  logFile.print(value2);
  logFile.print('; ');
  logFile.println(value3);
 
 
}
