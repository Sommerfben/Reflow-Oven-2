#include <LiquidCrystal.h>
#include "DFRkeypad.h"
#define DEBUG // uncomment this if you want verbose console logging

#define AREF 2.56           // set to AREF, typically board voltage like 3.3 or 5.0
#define ADC_RESOLUTION 10  // set to ADC bit resolution, 10 is default

//Pins
#define TC_PIN A1          // set thermocouple input to ADC pin used
#define Top_Element_Control 40  // set to relay control pin for the top element ACTIVE LOW
#define Bottom_Element__Control 41  // set to relay control pin for the bottom element ACTIVE LOW
#define estopPin 18       // E-stop interupt pin ACTIVE LOW

#define ON LOW 
#define OFF HIGH

//States
#define Wait_To_Start 0
#define Ramp_To_Soak 1
#define Soak 2
#define Ramp_To_Reflow 3
#define Reflow 4
#define Cool 5

//Buttons
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define ERR 4



//TODO: timeout safeties on heating cycles
 
enum ePins { LCD_RS=8, LCD_EN=9, LCD_D4=4, LCD_D5=5, LCD_D6=6, LCD_D7=7, LCD_BL=10 }; // define LCD pins
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7); // initialize the library with the numbers of the interface pins


//Durations are measured in seconds, temperatures in degrees C
float reading, voltage, temperature,total_temp, soak_time_start, reflow_time_start, oven_start_time;
int state = Wait_To_Start; 
float discarded_samples = 0;

//Durations in seconds
float Soak_Duration= 90; //90
float Reflow_Duration = 30; //45

//Temps in C 
float Soak_Temperature = 90; 
float Reflow_Temperature = 138; 
float Reset_Temperature = 35; 

//Correction factors
float soak_leave_early = 15; //Seconds
float soak_temp_overshoot = 25;  //degrees
float reflow_temp_overshoot = 15; //Degrees


float get_key_pressed(float voltage){
  if(voltage < 300 && voltage > 200){
    return UP; 
  }
  else if(voltage < 700 && voltage > 500){
    return DOWN; 
  }
  else if(voltage < 100){
    return RIGHT; 
  }
  else if(voltage < 950 && voltage > 800){
    return LEFT; 
  }
  else{
    return ERR;
  }
}


float get_voltage(int raw_adc) {
  return raw_adc * (AREF / (pow(2, ADC_RESOLUTION)-1));  
}
 
float get_temperature() {
  for(int i = 0; i < 100; i++){
    reading = analogRead(TC_PIN);
    voltage = get_voltage(reading);
    temperature = (voltage - 1.25) / 0.005;
    if(temperature > ((total_temp/i) + 5) || temperature < ((total_temp/i) - 5)){
      temperature = total_temp/i;
      discarded_samples = discarded_samples + 1; 
    }
    total_temp = total_temp + temperature; 
    delay(1);  
  }
  temperature = total_temp/100;
  total_temp = 0;
  return temperature;
}


void e_stop(){
  digitalWrite(Top_Element_Control, OFF);
  digitalWrite(Bottom_Element__Control, OFF);
  state = Wait_To_Start; 
  return;
}

void  thermo_couple_error(){
  digitalWrite(Top_Element_Control, OFF);
  digitalWrite(Bottom_Element__Control, OFF);
  state = Wait_To_Start; 
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print("TC ERROR");
  while(analogRead(0) > 1000){
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print("Press to reset");
        oven_start_time = millis();
        state = Wait_To_Start;
      }
  return;
}

void  timeout_error(){
  digitalWrite(Top_Element_Control, OFF);
  digitalWrite(Bottom_Element__Control, OFF);
  state = Wait_To_Start; 
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print("TIMEOUT");
  while(analogRead(0) > 1000){
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print("Press to reset");
        oven_start_time = millis();
        state = Wait_To_Start;
      }
  return;
}

void setup()
{
	#ifdef DEBUG
	Serial.begin(9600);
	//Serial.println("* Setup...");
	#endif
  pinMode(40, OUTPUT);    // sets the digital pin 13 as output
  pinMode(41, OUTPUT);    // sets the digital pin 13 as output
  lcd.begin(16, 2); 						                  // set up the LCD's number of columns and rows (16x2)


	pinMode(LCD_BL, OUTPUT); 			                  // pin LCD_BL is LCD backlight brightness (PWM)
	analogWrite(LCD_BL, 255);                       // set the PWM brightness to maximum
  lcd.setCursor(0, 0);
  lcd.print("Reflow Oven");
  pinMode(estopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(estopPin), e_stop, CHANGE);
  
  analogReference(INTERNAL2V56);
  #ifdef DEBUG
	//Serial.println("  Done.");
	#endif
}

void Display_Temperature(float temperature)
{
  //Serial.print("Temperature = ");
 // Serial.print(temperature);
  //Serial.println(" C");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("Temp: "); 
  lcd.print(temperature);                                               // print key name
  lcd.print("C");
}

void loop()
{
  temperature = get_temperature();
  if((temperature < 0 || temperature > 300) && millis() > 1000){
    thermo_couple_error();
  }
  else if((state != 0 || state != 5) && (millis() - oven_start_time) > (Soak_Duration + Reflow_Duration + 500.0)*1000){
    timeout_error();
  }
  


  //State machine
	if(state == Wait_To_Start ){
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print("Reflow Oven");
      digitalWrite(Top_Element_Control, OFF);
      digitalWrite(Bottom_Element__Control, OFF);
      while(analogRead(0) > 1000){
        lcd.print("                ");
        lcd.setCursor(0, 1);
        lcd.print("Press to begin");
        oven_start_time = millis();
        state = Ramp_To_Soak;
      }
    
	}
	else if(state == Ramp_To_Soak && (millis() > 1000)){ //Preheat State
    if (Soak_Temperature <= temperature){
       soak_time_start = millis();
       state = Soak;   
    }
    else{
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print("Preheat to ");
      lcd.print(int(Soak_Temperature));
      lcd.print("C");
      digitalWrite(Top_Element_Control, ON);
      digitalWrite(Bottom_Element__Control, ON);
    }
	}
  else if(state == Soak){  //Soaking State
    if ((millis() - soak_time_start) >= ((Soak_Duration * 1000) - soak_leave_early)){
       state = Ramp_To_Reflow;   
    }
    else{
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print("Soaking");
      lcd.print(" ");
      lcd.print((Soak_Duration - (millis() - soak_time_start)/1000));
      lcd.print("S");
      if((millis() - soak_time_start) < ((Soak_Duration * 1000)/1.3333)){
      digitalWrite(Top_Element_Control, OFF);
      digitalWrite(Bottom_Element__Control, OFF);
      }
      else{
        digitalWrite(Top_Element_Control, OFF);
        digitalWrite(Bottom_Element__Control, ON);
      }
    }
  }    
  else if (state == Ramp_To_Reflow){   //Ramp to reflow temp
    if (Reflow_Temperature <= temperature){
       state = Reflow; 
       reflow_time_start = millis();  
    }
    else{
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print("Ramp to ");
      lcd.print(int(Reflow_Temperature));
      lcd.print("C");
      digitalWrite(Top_Element_Control, ON);
      digitalWrite(Bottom_Element__Control, ON);
    }
  }
  else if (state == Reflow){   //Reflow
    if ((millis() - reflow_time_start) >= Reflow_Duration * 1000){
       state = Cool;   
    }
    else{
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print("Reflowing");
      lcd.print(" ");
      lcd.print((Reflow_Duration - (millis() - reflow_time_start)/1000));
      lcd.print("S");
      digitalWrite(Top_Element_Control, OFF);
      digitalWrite(Bottom_Element__Control, ON);
    }
  }
  else if (state == Cool){   //Cooling
    if (temperature <= Reset_Temperature){
       state = Wait_To_Start;   
    }
    else{
      lcd.setCursor(0, 0);
      lcd.print("                ");
      lcd.setCursor(0, 0);
      lcd.print("Cooling");
      digitalWrite(Top_Element_Control, OFF);
      digitalWrite(Bottom_Element__Control, OFF);
    }
  }


  //Serial.print("Currently in state: ");
  //Serial.println(state);
  //Serial.print("Discarded temperature samples: ");
  //Serial.println(discarded_samples);
  Serial.println(temperature);
  Display_Temperature(temperature); 
  delay(200);
} // void loop()
/*
  #ifdef DEBUG
  Serial.print("*  Key: ");
  Serial.println(DFRkeypad::KeyName(key));
  #endif 
  byte key=DFRkeypad::GetKey();                   // read a key identifier

  */
