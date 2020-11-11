/*
  Outside sensor for NIXIE Thermometer
  Ver 2.1

  Hardware required:
 - Temperature sensor DS18B20
 - 433 MHz radio transmitter (FS1000A)

 Autor: V.Nezlo
 E-mail: vlladimirka@gmail.com
 Github: https://github.com/V-Nezlo

*/

#include <iarduino_RF433_Transmitter.h>
#include <OneWire.h>
#include <GyverPower.h>

#define ADDRESS 111

OneWire ds(3);
iarduino_RF433_Transmitter radioTX(12);

int tempout;
int data[4];
int ds_data[2];
int k;


void check_sensors(void){

ds.reset();
ds.write(0xCC);
ds.write(0x44);

delay(1000);

ds.reset();
ds.write(0xCC);
ds.write(0xBE);

ds_data[0] = ds.read();
ds_data[1] = ds.read();

float temperature =  ((ds_data[1] << 8) | ds_data[0]) * 0.0625;

tempout=temperature*10;
}

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif

  delay(75); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

void setup(){

  radioTX.begin(1000);
  radioTX.openWritingPipe(5);
  power.setSleepMode(POWERDOWN_SLEEP);
  power.hardwareDisable(PWR_ADC);
  power.hardwareDisable(PWR_SPI);
  power.hardwareDisable(PWR_I2C);
  power.hardwareDisable(PWR_UART0);
  power.bodInSleep(false);
}

void loop(){

delay(100);
check_sensors();
data[0]=123;	//dumbass
data[1]=ADDRESS;//address
data[2]=tempout;//temperature

power.hardwareEnable(PWR_ADC);
data[3]=(readVcc())/10;   //voltage
power.hardwareDisable(PWR_ADC);

radioTX.write(&data, sizeof(data));
radioTX.write(&data, sizeof(data));
radioTX.write(&data, sizeof(data));
radioTX.write(&data, sizeof(data));
radioTX.write(&data, sizeof(data));

if (data[3]<350) power.sleep(SLEEP_FOREVER);


power.sleepDelay(30000);
}
