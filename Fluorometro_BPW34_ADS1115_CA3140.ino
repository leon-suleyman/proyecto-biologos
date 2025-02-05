#include <Adafruit_ADS1X15.h>
#include <Wire.h>

const uint8_t N_LUM_EXT_SAMPLES = 10;  //number of samples for average
unsigned long lumExtSample[11]; //[N_LUM_EXT_SAMPLES + 1] or [N_LUM_EXT_SAMPLES] ????
uint8_t lumExtIndex = 0;


uint16_t lumExt = 0;
uint16_t lumExtAvg = 0;

Adafruit_ADS1115 ads;
  
void setup(void) 
{
  Serial.begin(9600);
  delay(200);
 {
  for (uint8_t i = 0; i < 10; i++)
  {
    lumExtSample[i] = 0;  //init table to 0
  }
}
  // Cambiar factor de escala
  ads.setGain(GAIN_TWO);   // TWO me da 2,048V de referencia y 0,0625mV de escala
  
  // Iniciar el ADS1115
  ads.begin();
}
 
void loop(void) 
{
  short lumExt = ads.readADC_SingleEnded(0); // Obtener datos del A0 del ADS1115
  lumExtSample[lumExtIndex] = lumExt;  //put new value in table. lumExt is unit16_t and is comming from a sensor (from 0 to 35000).
  lumExtIndex++;
  if ( lumExtIndex >= N_LUM_EXT_SAMPLES ) //end of table ? 
  {
    lumExtIndex = 0;
  }

  for ( uint8_t i = 0 ; i < N_LUM_EXT_SAMPLES ; i++ ) //calculate mean value
  {
    lumExtAvg += lumExtSample[i];  //sum all elements
  }

  lumExtAvg /= N_LUM_EXT_SAMPLES;  //sum / n, or lumExtAvg /= uint16_t (N_LUM_EXT_SAMPLES);  ???

  //Serial.print(" lumExtAvg: "); 
  Serial.println(lumExtAvg); 
  delay(100);
}


 
