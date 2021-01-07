/*
  Погодная станция на NIXIE индикаторах
  Ver 6.6

  Требуемые железки:
 - Температурный датчик DS18B20
 - Датчик влажности DHT11 (DHT12,DHT21,DHT22)
 - 433 MHz радио-приемник (MX-RM-5V)

 Autor: V. Nezlo 
 E-mail: vlladimirka@gmail.com
 Github: https://github.com/V-Nezlo

*/

#include <iarduino_RF433_Receiver.h> 
#include <OneWire.h>
#include "DHT.h"
#include <GyverTimer.h>                  

OneWire ds(5); 		//ds18b20 пин
DHT dht(2, DHT11);	//DHT пин
iarduino_RF433_Receiver radioRX(3); //пин для радио (обязательно с хардварным прерыванием!)
#define ADDRESS 112
int data[4];
int ds_data[2];
uint8_t k;    

int tempin;  		         //температура*10
int tempout;  	         //температура снаружи *10
int voltage;             //Напряжение аккума внешнего датчика
char tempin_z; 		       //+ или - на первом индикаторе для внутренней температуры
char tempout_z;          //+ или - на первом индикаторе для внешней температуры
int humi;                //влажности
int digits[4];           //массив для цифр
char display_mode;       //режим
char display_mode_temp;  //тут храним режим при лечении катодов
char P=0;                //переменная для лечения

  bool connection=0; 			//флаг наличия соединения
  bool farenheit; 				//цельсии или фаренгейты
  bool farenheit_overload_in;   //flag for "carry" in
  bool farenheit_overload_out;  //flag for "carry" out
  
  const int out1 = A3;  //Пины декодера
  const int out2 = A1;  
  const int out3 = 13;  
  const int out4 = A2;  

  const int key1 = 9; 	//Пины для анодов
  const int key2 = 12; 	
  const int key3 = 10;	
  const int key4 = 11;	

  const int dot = 4; 	//Точка внутри лампы
  const int dot1= 6; 	//Точка внутренней температуры
  const int dot2= 7; 	//Точка внутренней влажности
  const int dot3= 8; 	//Точка внешней температуры

  bool dotf  =1;   //флаги точек
  bool dot1f =0;
  bool dot2f =0;
  bool dot3f =0;

GTimer Tconv_start(MS,1000);//Таймер начала конверсии в ds18
GTimer Tconv_read(MS,2000); //Таймер чтения температуры с ds18
GTimer Tmode_switch(MS,6000); //Таймер переключения режимов
GTimer Tcathode_heal(MS,180000);//Таймер для лечения катодов
GTimer Tcathode_switch(MS, 500);//Таймер для смены катодов во время лечения
GTimer Tled_lowbat(MS, 300);//Частота мигания индикатора при низком напряжении датчика
GTimer Tled_noconn(MS, 800);//Частота мигания индикатора при отсутствии соединения
GTimer Tvalidate_radio(MS, 600000); //Таймер для проверки валидации

const int bright = 1;  					//"Яркость" индикаторов

void radio_init(void){
	radioRX.begin(1000);                                  
    radioRX.openReadingPipe (5);                          
    radioRX.startListening  ();   
}

void check_radio(void){
    if(radioRX.available(&k)){              //Если в буфере что-то есть
    radioRX.read(&data, sizeof(data));      //Записать в массив
	if (data[1]==ADDRESS)							        //Проверить адрес
		{
		
		if (farenheit) tempout=(data[2]*1.8)+320;      //F=C*1,8+32
		else tempout=data[2];                            
		
		if (tempout>1000) farenheit_overload_out=1;
		else farenheit_overload_out=0;
		
		voltage=data[3];
		
		
		if (tempout>=0) tempout_z = 7;
		else tempout_z = 8;
		
    if (tempout<0) tempout = tempout * (-1);
		connection=true;
		Tvalidate_radio.reset();
		//Serial.println("Соединение установлено");
		}
	
  }
}

void check_validate_radio(void){
if (Tvalidate_radio.isReady())
	{
		connection=false;
		//Serial.print("Соединение потеряно");
	}
}

void setNumber(int num) { //Передача цифр в декодер
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

void show(int a[]){ //Установка разрядов
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
if (connection==false) {
    if (Tled_noconn.isReady())
	{
		dot1f = !dot1f;
		//Serial.println("мырг!");
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
  //Serial.print("Влажность: ");
  //Serial.println(humi);
}

void conversion_start(void){
  ds.reset(); 	  // Ресет
  ds.write(0xCC); // Пропуск скана шины
  ds.write(0x44); // Начать конверсию
}

void conversion_read(void){
  ds.reset(); // 
  ds.write(0xCC); 
  ds.write(0xBE);
  ds_data[0] = ds.read();
  ds_data[1] = ds.read(); 
 
  float temperature =  ((ds_data[1] << 8) | ds_data[0]) * 0.0625;

  if (farenheit) tempin=(temperature*18)+320;
  else tempin=temperature*10;
  
  if (tempin>1000) farenheit_overload_in = 1;
  else farenheit_overload_in = 0;
  
  if (tempin>0) tempin_z = 7;
  else tempin_z = 8;

  //Serial.print("Температура внутри ");
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
      display_mode = display_mode_temp; //Вернуть режим после лечения катодов
      Tmode_switch.resume();
      //Serial.println("Режим отображения восстановлен");
      P=0;
    }  
  }

	show(digits);
}

void switchMode(void){
if (Tmode_switch.isReady())
  {
    //Serial.println("Режим отображения изменен");
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
    display_mode_temp = display_mode;//Сохраняем текущий режим отображения
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
  check_sensors_first();//Первый запуск с делеем
  check_humidity();
  farenheit = digitalRead(A0);
  

  //Serial.print("Флаг фаренгейтов равен ");
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
