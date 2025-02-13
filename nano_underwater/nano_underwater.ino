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


const uint8_t oneWirePin = 2; //sensor dallas

OneWire oneWireBus(oneWirePin);
DallasTemperature sensor(&oneWireBus);


#define SSpin 10  // Pin 10, a CS/SS (Chip/Slave Select)



const uint8_t pinDatosDQ = 2;                         // Pin donde se conecta el bus l-wire
const uint8_t pinLed = 6; //led fluo
const uint8_t pinIntrpt = 3;
const uint8_t pin_rx_sim = 7;
const uint8_t pin_tx_sim = 9;

SoftwareSerial nano_sim(pin_rx_sim, pin_tx_sim);


Adafruit_ADS1115 ads;
const float multiplier = 0.1875F;

// RTC_DS1307 rtc;
RTC_DS3231 rtc;

//variables globales de los datos leidos
uint16_t ano_actual;
uint8_t mes_actual;
uint8_t dia_actual;
uint8_t ult_hora_lectura;
uint8_t lectura_hora[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t lectura_minuto[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t lectura_segundo[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t lectura_fluoro[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t lectura_irradiancia[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
int8_t lectura_temperatura[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

uint8_t indice_lectura = 0;

//Estados para cambiar entre sensar y comunicarse con el Arduino Nano con el SIM800L
enum : byte {READ, WRITE} estado = READ;


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
    attachInterrupt(digitalPinToInterrupt(pinIntrpt), reportarAlNanoSim, RISING);

    pinMode(pinLed, OUTPUT); // pin LED en output fluorom

    digitalWrite(6, LOW);

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
    DateTime now = rtc.now();
    ano_actual = now.year();
    mes_actual = now.month();
    dia_actual = now.day();
    ult_hora_lectura = now.hour();
  }
}   //termina el setup


void loop()
{
  //primero que nada consigo fecha y hora para usar.
  DateTime now = rtc.now();
  switch (estado){
    //Si estamos en modo lectura:
    case READ:
      //y tenemos espacio para escribir más cosas en nuestra memoria
      if(indice_lectura <= 12){
        digitalWrite(6, HIGH);
        //delay(120000); //2 min para el led
    
      // guardar hora actual del sensado
        lectura_hora[indice_lectura] = now.hour();
        lectura_minuto[indice_lectura] = now.minute();
        lectura_segundo[indice_lectura] = now.second();
    
      // lee Fluorescencia
        lectura_fluoro[indice_lectura] = readSensorFluoro();

      // lee irradiancia
        lectura_irradiancia[indice_lectura] = readSensorIrradiancia();

      // lee temperatura:
        lectura_temperatura[indice_lectura] = readSensorTemperatura();

        indice_lectura++;

        digitalWrite(6, LOW);
      }
      break;

    case WRITE:

      for(int i = 0; i < indice_lectura; i++){
        //si cambió el día entonces es el actual
        if(lectura_hora[i] < ult_hora_lectura){
          ano_actual = now.year();
          mes_actual = now.month();
          dia_actual = now.day();
        }
        //armo el string que voy a pasarle al nano_sim
        String lectura_txt = "";
        lectura_txt = lectura_txt + String(ano_actual) + "/" + String(mes_actual) + "/" + String(dia_actual) + " " + String(lectura_hora[i]) + ":" + String(lectura_minuto[i]) + ":" + String(lectura_segundo[i]) + ";";
        lectura_txt = lectura_txt + String(lectura_fluoro[i]) + ";" + String(lectura_irradiancia[i]) + ";" + String(lectura_temperatura[i]);
        //se lo paso por software serial
        nano_sim.println(lectura_txt);

        ult_hora_lectura = lectura_hora[i];

        delay(100);

        if(_readSerialSIM() != "llegó"){
          estado = READ;
          indice_lectura = 0;
          break;
        }
      }
      //si terminé sin errores, cambio a READ y hago un reseteo del programa para limpiar la memoria.
      if(estado == WRITE){
        estado = READ;
        indice_lectura = 0;
        software_Reset();
      }

      break;
  }

    delay(1000); // 60 segundos (TIEMPO de delay LOOP)
}
//termina el PP

//desde aca Funciones declaracion:

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
asm volatile ("  jmp 0");  
}

//cambia el estado cuando le manda una interrupción el nano con la SIM, ahora va a pasarle las lecturas a ese nano.
void reportarAlNanoSim(){
  estado = WRITE;
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


