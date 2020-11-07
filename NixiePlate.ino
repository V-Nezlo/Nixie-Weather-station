/*
  Weather station based on NIXIE lamps
  Ver 6.4

  Hardware required:
 - Temperature sensor DS18B20
 - Humidity sensor DHT11 (DHT12,DHT21,DHT22)
 - 433 MHz radio receiver (MX-RM-5V)

 Autor: V. Nezlo 
 E-mail: vlladimirka@gmail.com
 Github: https://github.com/V-Nezlo

*/

//radio
#include <iarduino_RF433_Receiver.h>                      
iarduino_RF433_Receiver    radioRX(3);                    
int data[4];
uint8_t k;
#define ADDRESS 111
//radio

#include <OneWire.h>
#include "DHT.h"
#include <GyverTimer.h>                  

OneWire ds(5); 		//ds18b20 pin 
DHT dht(2, DHT11);	//DHT11 (or another) pin 

int tempin;  		// temp*10 
int tempout;  	    // out temp *10
int voltage;    // voltage of receiver' battery
char tempin_z; 		// + or - on indicators
char tempout_z;     // + or - on indicators
int humi;           // humidity
int digits[4];      // nums
char display_mode;  //mode
char display_mode_temp; //for cathode healing
char P=0; //for carhode healing

  bool connection=0; 			//connection flag
  bool farenheit; 				//celsium or farenheit
  bool farenheit_overload_in;   //flag for "carry" in
  bool farenheit_overload_out;  //flag for "carry" out
  
  const int out1 = A3;  //Decoder pin 1
  const int out2 = A1;  //Decoder pin 2
  const int out3 = 13;  //Decoder pin 3
  const int out4 = A2;  //Decoder pin 4

  const int key1 = 9; 	//Anode pin 1
  const int key2 = 12; 	//Anode pin 2
  const int key3 = 10;	//Anode pin 3
  const int key4 = 11;	//Anode pin 4

  const int dot = 4; 	//Dot inside the lamp
  const int dot1= 6; 	//Dot of internal temp
  const int dot2= 7; 	//Dot of internal humi
  const int dot3= 8; 	//Dot of external temp

  bool dotf  =1;   //flags
  bool dot1f =0;
  bool dot2f =0;
  bool dot3f =0;

GTimer Tconv_start(MS,1000);//Start conversion in ds18
GTimer Tconv_read(MS,2000); //Read from ds18 time
GTimer Tmode_switch(MS,6000); //switch mode time
GTimer Tcathode_heal(MS,180000);//cathode healing time
GTimer Tcathode_switch(MS, 500);//cathode healing switch time
GTimer Tled_lowbat(MS, 300);//led switching (low battery)
GTimer Tled_noconn(MS, 800);//led switching (noconnection)
GTimer Tvalidate_radio(MS, 600000); //time to validate data

const int bright = 1;  					// Value of "bright" of indicators

void radio_init(void){
	radioRX.begin(1000);                                  
    radioRX.openReadingPipe (5);                          
    radioRX.startListening  ();   
}

void check_radio(void){
    if(radioRX.available(&k)){                         //If buffer has something
    radioRX.read(&data, sizeof(data));                 //Read in data[]
	if (data[1]==ADDRESS)							   //Address checking
		{
		
		if (farenheit) tempout=(data[2]*1.8)+320;      //F=C*1,8+32
		else tempout=data[2];                            
		
		if (tempout>1000) farenheit_overload_out=1;
		else farenheit_overload_out=0;
		
		voltage=data[3];
		
		
		if (tempout>0) tempout_z = 7;
		else tempout_z = 8;
		
		//dot1f = 1;
		
		connection=TRUE;
		Tvalidate_radio.reset();
		//Serial.println("Connection established");
		}
	
    }
}

void check_validate_radio(void){
if (Tvalidate_radio.isReady())
	{
		connection=FALSE;
		//Serial.print("Connection lost and value is ");
		//Serial.println(connection);
	}
}

void setNumber(int num) { // Transfer number to the decoder
  switch (num)
  {
    case 0:
    digitalWrite (out1,LOW);
    digitalWrite (out2,LOW);
    digitalWrite (out3,LOW);
    digitalWrite (out4,LOW);
    break;
    case 1:
    digitalWrite (out1,HIGH);
    digitalWrite (out2,LOW);
    digitalWrite (out3,LOW);
    digitalWrite (out4,LOW);
    break;
    case 2:
    digitalWrite (out1,LOW);
    digitalWrite (out2,HIGH);
    digitalWrite (out3,LOW);
    digitalWrite (out4,LOW);
    break;
    case 3:
    digitalWrite (out1,HIGH);
    digitalWrite (out2,HIGH);
    digitalWrite (out3,LOW);
    digitalWrite (out4,LOW);
    break;
    case 4:
    digitalWrite (out1,LOW);
    digitalWrite (out2,LOW);
    digitalWrite (out3,HIGH);
    digitalWrite (out4,LOW);
    break;
    case 5:
    digitalWrite (out1,HIGH);
    digitalWrite (out2,LOW);
    digitalWrite (out3,HIGH);
    digitalWrite (out4,LOW);
    break;
    case 6:
    digitalWrite (out1,LOW);
    digitalWrite (out2,HIGH);
    digitalWrite (out3,HIGH);
    digitalWrite (out4,LOW);
    break;
    case 7:
    digitalWrite (out1,HIGH);
    digitalWrite (out2,HIGH);
    digitalWrite (out3,HIGH);
    digitalWrite (out4,LOW);
    break;
    case 8:
    digitalWrite (out1,LOW);
    digitalWrite (out2,LOW);
    digitalWrite (out3,LOW);
    digitalWrite (out4,HIGH);
    break;
    case 9:
    digitalWrite (out1,HIGH);
    digitalWrite (out2,LOW);
    digitalWrite (out3,LOW);
    digitalWrite (out4,HIGH);
    break;
  }
}

void show(int a[]){ //Set numbers on indicators
	int keys[] = { key1, key2, key3, key4 };
	for (int i=0; i<4; ++i){
		setNumber(a[i]);
		digitalWrite(keys[i], LOW);
		delay(bright);
		digitalWrite(keys[i], HIGH);
	}

  digitalWrite(dot, dotf);
  digitalWrite(dot1, dot1f);
  digitalWrite(dot2, dot2f);
  digitalWrite(dot3, dot3f);
}

void pin_set(void){
	
	pinMode(out1,OUTPUT);
	pinMode(out2,OUTPUT);
	pinMode(out3,OUTPUT);
	pinMode(out4,OUTPUT);

	pinMode(key1,OUTPUT);
	pinMode(key2,OUTPUT);
	pinMode(key3,OUTPUT);
	pinMode(key4,OUTPUT);

	pinMode(dot,OUTPUT);
	pinMode(dot1,OUTPUT);
	pinMode(dot2,OUTPUT);
	pinMode(dot3,OUTPUT);
	pinMode(A0, INPUT);

  digitalWrite (key1,HIGH);
  digitalWrite (key2,HIGH);
  digitalWrite (key3,HIGH);
  digitalWrite (key4,HIGH);
}

void led_blinking(void){
if (connection==FALSE) {
    if (Tled_noconn.isReady())
	{
		dot1f = !dot1f;
		//Serial.println("blink!");
	} 
  }
else if (voltage<370){
     if (Tled_lowbat.isReady()) dot1f = !dot1f;
  }
}

void check_sensors(void){

  if (Tconv_start.isReady()) conversion_start();
  if (Tconv_read.isReady())  conversion_read();
}

void check_sensors_first(void){

  conversion_start();
  delay(1000);
  conversion_read();
}

void check_humidity(void){
  float h = dht.readHumidity();
  humi=h*10;
  //Serial.print("Humidity is ");
  //Serial.println(humi);
}

void conversion_start(void){
  ds.reset(); 	  // ALL RESET
  ds.write(0xCC); // Skip device search
  ds.write(0x44); // Start conversion
}

void conversion_read(void){
  //Serial.println("Im start lagging boiii");
  ds.reset(); // 
  ds.write(0xCC); 
  ds.write(0xBE);
  data[0] = ds.read();
  data[1] = ds.read(); 
   //Serial.println("Im done");
 
  float temperature =  ((data[1] << 8) | data[0]) * 0.0625;

  if (farenheit) tempin=(temperature*18)+320;
  else tempin=temperature*10;
  
  if (tempin>1000) farenheit_overload_in = 1;
  else farenheit_overload_in = 0;
  
  if (tempin>0) tempin_z = 7;
  else tempin_z = 8;

  //Serial.print("In temperature is ");
  //Serial.println(tempin);
}

void displayMode(void){
  if ((display_mode==0)&(farenheit_overload_in==0))
  {
  digits[0] = tempin_z; // + -
  digits[1] = tempin/100; 
  digits[2] = tempin%100/10; 
  digits[3] = tempin%10; 

  dotf=1;
  
  if (connection){
	if (voltage>=370) dot1f=0;
	}
	
  dot2f=0;
  dot3f=1;
  }
  
    if ((display_mode==0)&(farenheit_overload_in==1))
  {
  digits[0] = tempin_z; // + -
  digits[1] = tempin/1000; 
  digits[2] = tempin%1000/100; 
  digits[3] = tempin%100/10;

  dotf=0;
  if (connection){
	if (voltage>=370) dot1f=0;
	}
  dot2f=0;
  dot3f=1;
  }

  if (display_mode==1)
  {
  digits[0] = 2; // %
  digits[1] = humi/100; 
  digits[2] = humi%100/10; 
  digits[3] = humi%10; 

  dotf=1;
  if (connection){
	if (voltage>=370) dot1f=0;
	}
  dot2f=1;
  dot3f=0;
  }

  if ((display_mode==2)&(farenheit_overload_out==0))
  {
  digits[0] = tempout_z; // + -
  digits[1] = tempout/100; 
  digits[2] = tempout%100/10; 
  digits[3] = tempout%10; 

  dotf=1;
  if (connection){
	if (voltage>=370) dot1f=1;
	}
  dot2f=0;
  dot3f=0;
  }
  
    if ((display_mode==2)&(farenheit_overload_out==1))
  {
  digits[0] = tempout_z; // + -
  digits[1] = tempout/1000; 
  digits[2] = tempout%1000/100; 
  digits[3] = tempout%100/10; 

  dotf=0;
  if (connection){
	if (voltage>=370) dot1f=1;
	}
  dot2f=0;
  dot3f=0;
  }

    if (display_mode==3)
  {

  digits[0] = P;
  digits[1] = P; 
  digits[2] = P; 
  digits[3] = P;

    if(P==0||P==3||P==6)dot1f=1,dot2f=0,dot3f=0;
    if(P==1||P==4||P==7)dot1f=0,dot2f=1,dot3f=0;
    if(P==2||P==5||P==8)dot1f=0,dot2f=0,dot3f=1;
    if(P==9)dot1f=1,dot2f=1,dot3f=1;

    if (Tcathode_switch.isReady()) P++; 
    if (P>9) 
    {
      display_mode = display_mode_temp;
      Tmode_switch.resume();
      //Serial.println("Display mode restored");
      P=0;
    }  
  }

	show(digits);
}

void switchMode(void){
if (Tmode_switch.isReady())
  {
    //Serial.println("Display mode switched");
    switch (display_mode)
    {
      case 0:
      display_mode=1;
      check_humidity();
      break;

      case 1:
      if (connection) display_mode=2;
      else display_mode=0;
      check_sensors();
      delay(30);
      break;

      case 2:
	  //Serial.println("Current mode is out,switching to in");
      display_mode=0;
	  delay(30);
      break;
    }
  }
}

void cathodeHeal(void){
 if (Tcathode_heal.isReady())
  {
    Tmode_switch.reset();
    display_mode_temp = display_mode;//save display_mode value
    display_mode=3;
    Tmode_switch.stop();
    //Serial.println("Cathode healing enable");
  }

}

void setup(){

  Serial.begin(9600);
  radio_init();
  pin_set();
  dht.begin();
  delay(300);
  check_sensors_first();//first run with delay
  check_humidity();
  farenheit = digitalRead(A0);
  

  //Serial.print("farenheit is ");
  //Serial.println(farenheit);
  
  display_mode=0;
}
  
void loop(){
  switchMode();
  displayMode();
  check_radio();
  check_validate_radio();
  led_blinking();
  cathodeHeal();
}





