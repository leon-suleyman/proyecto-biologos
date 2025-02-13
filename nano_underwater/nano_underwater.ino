// Programa incluye lectura 3x fotodiodos (inferior: fluorimetro para clorofila + turbidez; superior: irradiancia solar) y 1x DS18B20 para T°.
// Modulo RTC DS1302, modulo tarjeta SD, LED azul (fluorescencia), LED rojo (encendido).
// Modulo ADS1115 y OpAmp CA3140.
// Modulo Reed Switch.
// Amplitud lectura para fluorescencia GAIN_FOUR.
// Instalar modo Sleep para colecta de datos cada hora.

// pines:
// D2: Dallas DS18b20
// D3: pin para mandar interrupción al nano con la SIM800L
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


const uint8_t oneWirePin = 2; //sensor dallas

OneWire oneWireBus(oneWirePin);
DallasTemperature sensor(&oneWireBus);


#define SSpin 10  // Pin 10, a CS/SS (Chip/Slave Select)



const uint8_t pinDatosDQ = 2;                         // Pin donde se conecta el bus l-wire
const uint8_t pinLed = 6; //led fluo
const uint8_t pin_interrupt_nano_sim = 3;
const uint8_t pin_rx_sim = 7;
const uint8_t pin_tx_sim = 9;

SoftwareSerial nano_sim(pin_rx_sim, pin_tx_sim);


Adafruit_ADS1115 ads;
const float multiplier = 0.1875F;

// RTC_DS1307 rtc;
RTC_DS3231 rtc;


void setup()
{
  {
    Serial.begin(9600);
    Serial.print("Iniciando Sistema... ");

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
    //nano_sim.println("Buen día!");

    pinMode(pinLed, OUTPUT); // pin LED en output fluorom
    pinMode(pin_interrupt_nano_sim, OUTPUT);

    digitalWrite(6, LOW);
    digitalWrite(pin_interrupt_nano_sim, LOW);

    Serial.println("Completado");
 
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
  //primero que nada consigo fecha y hora para usar.
  DateTime now = rtc.now();

  digitalWrite(6, HIGH);
  //delay(120000); //2 min para el led

// lee Fluorescencia
  int fluoro = readSensorFluoro();

// lee irradiancia
  int irradiancia = readSensorIrradiancia();

// lee temperatura:
  int temperatura = readSensorTemperatura();

  digitalWrite(6, LOW);
  //mando interrupción al nano SIM para que me escuche los datos que mando;
  digitalWrite(pin_interrupt_nano_sim, HIGH);
  bool recibido = false;
  int timeOld = millis();
  while(!recibido && (millis() <= timeOld + 5000)){
    //armo el string que voy a pasarle al nano_sim
    String lectura_txt = "";
    lectura_txt = lectura_txt + String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + ";";
    lectura_txt = lectura_txt + String(fluoro) + ";" + String(irradiancia) + ";" + String(temperatura);
    //se lo paso por software serial
    nano_sim.println(lectura_txt);
    delay(100);
    if(_readSerialSIM() == "llegó"){
      recibido = true;
    }

  digitalWrite(pin_interrupt_nano_sim, LOW);

  }
  
  delay(1000); // 60 segundos (TIEMPO de delay LOOP)

  software_Reset();

}
//termina el PP

//desde aca Funciones declaracion:

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
asm volatile ("  jmp 0");  
}

//lee la comunicación serial con el arduino nano con el sim800L
String _readSerialSIM(){
  uint64_t timeOld = millis();

  while (!nano_sim.available() && !(millis() > timeOld + 5000))
  {
      delay(13);
  }

  String str;

  while(nano_sim.available())
  {
      if (nano_sim.available()>0)
      {
          str += (char) nano_sim.read();
      }
  }

  return str;
}

// Funcion sensores
int readSensorIrradiancia()
  {
  int sval = 0;
  ads.setGain(GAIN_ONE);      //  +/- 4.096V  1 bit = 0.125mV
  ads.begin();
  //int16_t adc0;
  // int16_t adc0, adc1;

 
  for (int i = 0; i < 5; i++){
    sval += ads.readADC_SingleEnded(1);   // sensor on analog pin 0
    delay(100);
  }

  sval = sval / 5;    //promedio de las 5 medidas tomadas cada 100 ms

  Serial.print("AIN1: "); Serial.println(sval);  //solo imprime por serie el 1 er valor - es para testeo unicamente -
  delay(10);
  return sval;
}


int readSensorTemperatura()
{
  int TempDallas = 0;
  sensor.requestTemperatures();
  Serial.print("Temperatura en sensor 0: ");
  TempDallas = sensor.getTempCByIndex(0);
  Serial.print(TempDallas);
  Serial.println(" ºC");
  return TempDallas;
}


int readSensorFluoro()
// el led debe estar prendido 2 min antes
{
  ads.setGain(GAIN_FOUR);    //   +/- 1.024V  1 bit = 0.03125mV
  ads.begin();
  int sval0 = 0;
  ads.setGain(GAIN_ONE);      //  +/- 4.096V  1 bit = 0.125mV

 
  for (int i = 0; i < 5; i++){
    sval0 += ads.readADC_SingleEnded(1);   // sensor on analog pin 0
    delay(100);
  }

  sval0 = sval0 / 5;    //promedio de las 5 medidas tomadas cada 100 ms

  Serial.print("AIN0: "); Serial.println(sval0);  //solo imprime por serie el valor - es para testeo unicamente -
  delay(10);
  return sval0;
 
}


