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


#include <OneWire.h>               // Incluir promagra OneWire lectura
#include <DallasTemperature.h>   // Incluir programa DallaTemperature lectura
#include <Adafruit_ADS1X15.h>  // incluye libreria conversor ADS1115
#include <Wire.h>   // incluye libreria para lector temperatura DS18B20
#include <RTClib.h>   // incluye libreria para el manejo del modulo RTC DS3231
#include <SoftwareSerial.h>

#include <SPI.h>
#include <SdFat.h>
SdFat SD;
File datos_actuales;


const uint8_t oneWirePin = 2; //sensor dallas

OneWire oneWireBus(oneWirePin);
DallasTemperature sensor(&oneWireBus);


#define SSpin 10  // Pin 10, a CS/SS (Chip/Slave Select)



#define PIN_DATOS_DQ 2                          // Pin donde se conecta el bus l-wire
#define PIN_LED 6                               //led fluo
#define PIN_INTRPT_NANO_SIM 3                   //pin por donde se manda la interrupción al arduino nano que tiene la SIM800L
#define PIN_RX_SIM 7                            //pin de RX con el arduino sim
#define PIN_TX_SIM 9                            //pin de TX con el arduino sim


SoftwareSerial nano_sim(PIN_RX_SIM, PIN_TX_SIM);
char buffer_int[5];
//char lectura_txt[40];


Adafruit_ADS1115 ads;
const float multiplier = 0.1875F;

// RTC_DS1307 rtc;
RTC_DS3231 rtc;

const uint8_t this_nano_id = 0;                    //ID para el nano con sensores. Habría que cambiar para cada uno.

//enum : byte {TOMANDO_DATOS, ENVIAR_DATOS} estado = TOMANDO_DATOS;

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

    pinMode(PIN_LED, OUTPUT); // pin LED en output fluorom
    pinMode(PIN_INTRPT_NANO_SIM, OUTPUT);

    digitalWrite(PIN_LED, LOW);
    //attachInterrupt(digitalPinToInterrupt(PIN_INTRPT_NANO_SIM), interrupcionNano, RISING);
    digitalWrite(PIN_INTRPT_NANO_SIM, LOW);

    Serial.println("Completado");

    
    //inicialización de la tarejta SD
    Serial.print("Initializing SD card...");

    if (!SD.begin(SSpin)) {
      Serial.println("initialization failed!");
      return;
    }
    Serial.println("initialization done.");
    
 
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
  /*
  switch(estado){
    case TOMANDO_DATOS:
      //primero que nada consigo fecha y hora para usar.
      DateTime now = rtc.now();

      digitalWrite(PIN_LED, HIGH);
      //delay(120000); //2 min para el led

    // lee Fluorescencia
      int16_t fluoro = readSensorFluoro();

    // lee irradiancia
      int16_t irradiancia = readSensorIrradiancia();

    // lee temperatura:
      int16_t temperatura = readSensorTemperatura();

      digitalWrite(PIN_LED, LOW);
      //mando interrupción al nano SIM para que me escuche los datos que mando;
      digitalWrite(PIN_INTRPT_NANO_SIM, HIGH);
      bool recibido = false;
      int timeOld = millis();
      lectura_txt[0] = NULL;
      
      //armo el string que voy a pasarle al nano_sim
      strcat(lectura_txt, intToCString(this_nano_id));
      strcat(lectura_txt, ";" );
      strcat(lectura_txt, intToCString(now.year()) );
      strcat(lectura_txt, "/" );
      strcat(lectura_txt, intToCString(now.month()) );
      strcat(lectura_txt, "/" );
      strcat(lectura_txt, intToCString(now.day()) );
      strcat(lectura_txt, " " );
      strcat(lectura_txt, intToCString(now.hour()) );
      strcat(lectura_txt, ":" );
      strcat(lectura_txt, intToCString(now.minute()) );
      strcat(lectura_txt, ":" );
      strcat(lectura_txt, intToCString(now.second()) );
      strcat(lectura_txt, ";" );
      strcat(lectura_txt, intToCString(fluoro) );
      strcat(lectura_txt, ";" );
      strcat(lectura_txt, intToCString(irradiancia) );
      strcat(lectura_txt, ";" );
      strcat(lectura_txt, intToCString(temperatura) );
      strcat(lectura_txt, "\n" );
      
      Serial.print(lectura_txt);

      digitalWrite(PIN_INTRPT_NANO_SIM, LOW);

      //anotamos en la tarjeta SD la lectura
      char filename[22];
      filename[0] = NULL;
      strcat(filename, "data_");
      strcat(filename, intToCString(now.year()));
      strcat(filename, "_");
      strcat(filename, intToCString(now.month()));
      strcat(filename, "_");
      strcat(filename, intToCString(now.day()));
      strcat(filename, ".csv");

      bool no_existe_previamente = true;
      if(SD.exists(filename)){
        no_existe_previamente = false;
      }

      datos_actuales = SD.open(filename, FILE_WRITE);
      if (datos_actuales) {
        Serial.print("Writing data...");
        if(no_existe_previamente){
          datos_actuales.println("ID;DateTime;fluoro;irradiancia;temperatura");
        }
        datos_actuales.print(lectura_txt);
        // close the file:
        datos_actuales.close();
        Serial.println("done.");
      } else {
        // if the file didn't open, print an error:
        Serial.println("error opening file");
      }

      //escribo en el archivo con todos los datos que no se enviaron al nano sim todavía
      datos_actuales = SD.open("latest_data.csv", FILE_WRITE);
      if (datos_actuales) {
        Serial.print("Writing data...");
        datos_actuales.print(lectura_txt);
        // close the file:
        datos_actuales.close();
        Serial.println("done.");
      } else {
        // if the file didn't open, print an error:
        Serial.println("error opening file: latest_data.csv");
      }

      delay(5000); // 60 segundos (TIEMPO de delay LOOP)
    break;

    case ENVIAR_DATOS:
      lectura_txt[0] = NULL;

      if(SD.exists("latest_data.csv")){
        datos_actuales = SD.open("latest_data.csv");
        if (datos_actuales) {
          Serial.print("Reading data...");
          while(datos_actuales.available()){
            char temp[2];
            char c = (char) nano_sim.read();
            temp[1] = NULL;
            temp[0] = c;

            strcat(lectura_txt, temp);

            if(c == '\n'){
              nano_sim.print(lectura_txt);
              delay(100);
              int timeOld = millis();
              //esperamos que nos responda que llegó el nano SIM
              while(_readSerialSIM() != "llegó" && (millis() > timeOld + 5000)){
                nano_sim.print(lectura_txt);
              }

              lectura_txt[0] = NULL;
            }
          }

          datos_actuales.close();
          //eliminamos el archivo para no enviar de nuevo esta info por mensaje
          SD.remove("latest_data.csv");
          Serial.println("done.");
        } else {
          // if the file didn't open, print an error:
          Serial.println("error opening file");
        }
      }
      //vuelvo a tomar datos normalmente
      estado = TOMANDO_DATOS;

    break;
  }
  */
  
  //primero que nada consigo fecha y hora para usar.
  DateTime now = rtc.now();

  digitalWrite(PIN_LED, HIGH);
  //delay(120000); //2 min para el led

// lee Fluorescencia
  int16_t fluoro = readSensorFluoro();

// lee irradiancia
  int16_t irradiancia = readSensorIrradiancia();

// lee temperatura:
  int16_t temperatura = readSensorTemperatura();

  digitalWrite(PIN_LED, LOW);
  bool recibido = false;
  int timeOld = millis();
  char lectura_txt[40];
  lectura_txt[0] = NULL;
  
  //armo el string que voy a pasarle al nano_sim
  strcat(lectura_txt, intToCString(this_nano_id));
  strcat(lectura_txt, ";" );
  strcat(lectura_txt, intToCString(now.year()) );
  strcat(lectura_txt, "/" );
  strcat(lectura_txt, intToCString(now.month()) );
  strcat(lectura_txt, "/" );
  strcat(lectura_txt, intToCString(now.day()) );
  strcat(lectura_txt, " " );
  strcat(lectura_txt, intToCString(now.hour()) );
  strcat(lectura_txt, ":" );
  strcat(lectura_txt, intToCString(now.minute()) );
  strcat(lectura_txt, ":" );
  strcat(lectura_txt, intToCString(now.second()) );
  strcat(lectura_txt, ";" );
  strcat(lectura_txt, intToCString(fluoro) );
  strcat(lectura_txt, ";" );
  strcat(lectura_txt, intToCString(irradiancia) );
  strcat(lectura_txt, ";" );
  strcat(lectura_txt, intToCString(temperatura) );
  strcat(lectura_txt, "\n" );

  //mando interrupción al nano SIM para que me escuche los datos que mando;
  digitalWrite(PIN_INTRPT_NANO_SIM, HIGH);

  /*
  while(!recibido && (millis() <= timeOld + 5000)){
    //se lo paso por software serial
    nano_sim.print(lectura_txt);
    delay(100);
    if(_readSerialSIM() == "llegó"){
      recibido = true;
    }else{
      delay(500);
    }
  }
  */
  nano_sim.print(lectura_txt);

  Serial.print(lectura_txt);

  digitalWrite(PIN_INTRPT_NANO_SIM, LOW);

  
  //anotamos en la tarjeta SD la lectura
  char filename[22];
  filename[0] = NULL;
  strcat(filename, "data_");
  strcat(filename, intToCString(now.year()));
  strcat(filename, "_");
  strcat(filename, intToCString(now.month()));
  strcat(filename, "_");
  strcat(filename, intToCString(now.day()));
  strcat(filename, ".csv");

  bool no_existe_previamente = true;
  if(SD.exists(filename)){
    no_existe_previamente = false;
  }

  datos_actuales = SD.open(filename, FILE_WRITE);
  if (datos_actuales) {
    Serial.print("Writing data...");
    if(no_existe_previamente){
      datos_actuales.println("ID;DateTime;fluoro;irradiancia;temperatura");
    }
    datos_actuales.print(lectura_txt);
    // close the file:
    datos_actuales.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening file");
  }
  

  delay(5000); // 60 segundos (TIEMPO de delay LOOP)
  software_Reset();

  /*
  if(estado != ENVIAR_DATOS){
    software_Reset();
  }
  */

}
//termina el PP

//desde aca Funciones declaracion:

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
asm volatile ("  jmp 0");  
}

//interrupción
/*
void interrupcionNano(){
  estado = ENVIAR_DATOS;
}
*/

//lee la comunicación serial con el arduino nano con el sim800L

char* _readSerialSIM(){
  uint64_t timeOld = millis();

  while (!nano_sim.available() && !(millis() > timeOld + 5000))
  {
      delay(13);
  }

  char str[64];
  str[0] = NULL;
  char temp[2];

  while(nano_sim.available())
  {
      if (nano_sim.available()>0)
      {
          //str += (char) nano_sim.read();
          temp[1] = NULL;
          temp[0] = (char) nano_sim.read();

          strcat(str, temp);
      }
  }

  return str;
}

//aux
char* intToCString(int x){
  buffer_int[0] = NULL;
  char buff_buffer[5];
  buff_buffer[0] = NULL;
  if(x < 0){
    strcat(buff_buffer, "-");
  }

  itoa(x, buff_buffer, 10);

  strcat(buffer_int, buff_buffer);
  return buffer_int;
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


