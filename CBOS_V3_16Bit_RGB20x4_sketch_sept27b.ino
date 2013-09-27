

/*
   This example code is in the public domain.
   ---------------------------------------------------------------------
   This program demonstrates ADS115 16-Bit ADC read in the Diif mode. button detection, LCD text/number printing,
  and LCD backlight control on the Freetronics LCD & Keypad Shield, connected to an Arduino board.
  
  Pins used by LCD & Keypad Shield:
 
    A0: Buttons, analog input from voltage ladder
    D4: LCD bit 4
    D5: LCD bit 5
    D6: LCD bit 6
    D7: LCD bit 7
    D8: LCD RS
    D9: LCD E
    D10: LCD Backlight (high = on, also has pullup high so default is on)
 
 10-BIT ADC voltages for the 5 buttons on analog input pin A0:
 
    RIGHT:  0.00V :   0 @ 8bit ;   0 @ 10 bit
    UP:     0.71V :  36 @ 8bit ; 145 @ 10 bit
    DOWN:   1.61V :  82 @ 8bit ; 329 @ 10 bit
    LEFT:   2.47V : 126 @ 8bit ; 505 @ 10 bit
    SELECT: 3.62V : 185 @ 8bit ; 741 @ 10 bit

/*--------------------------------------------------------------------------------------
  Includes
--------------------------------------------------------------------------------------*/
#include <LiquidCrystal.h>   // include LCD library
#include <Wire.h>
#include <Adafruit_ADS1015.h>
Adafruit_ADS1015 ads1115 (0x48);
#include <LCDKeypad.h>

/*--------------------------------------------------------------------------------------
  Defines
--------------------------------------------------------------------------------------*/
// Pins in use
#define BUTTON_ADC_PIN           A0  // A0 is the button ADC input
// #define RED_BACKLIGHT_PIN         3  // D3 controls LCD PWM RED backlight
//  #define GRN_BACKLIGHT_PIN         5  // D10 controls LCD PWM GREEN backlight
//  #define BLU_BACKLIGHT_PIN         6  // D10 controls LCD PWM BLUE backlight
#define RAM_DIRECTION_PIN      13  // D13 controls SSR to drive Ram direction relay
// ADC readings expected for the 5 buttons on the ADC input
#define RIGHT_10BIT_ADC           0  // right
#define UP_10BIT_ADC            145  // up
#define DOWN_10BIT_ADC          329  // down
#define LEFT_10BIT_ADC          505  // left
#define SELECT_10BIT_ADC        741  // right
#define BUTTONHYSTERESIS         10  // hysteresis for valid button sensing window
//return values for ReadButtons()
#define BUTTON_NONE               0  // 
#define BUTTON_RIGHT              1  // 
#define BUTTON_UP                 2  // 
#define BUTTON_DOWN               3  // 
#define BUTTON_LEFT               4  // 
#define BUTTON_SELECT             5  // 

#define REDLITE 3
#define GREENLITE 5
#define BLUELITE 6 


//some example macros with friendly labels for LCD backlight/pin control, tested and can be swapped into the example code as you like
// #define LCD_BACKLIGHT_OFF()     digitalWrite( LCD_BACKLIGHT_PIN, LOW )
// #define LCD_BACKLIGHT_ON()      digitalWrite( LCD_BACKLIGHT_PIN, HIGH )
// #define LCD_BACKLIGHT(state)    { if( state ){digitalWrite( LCD_BACKLIGHT_PIN, HIGH );}else{digitalWrite( LCD_BACKLIGHT_PIN, LOW );} }

#define RAM_Extend()     digitalWrite( RAM_DIRECTION_PIN, LOW )
#define RAM_Retract()      digitalWrite( RAM_DIRECTION_PIN, HIGH )
#define RAM_DIRECTION(state)    { if( state ){digitalWrite( RAM_DIRECTION_PIN, HIGH );}else{digitalWrite( RAM_DIRECTION_PIN, LOW );} }
/*--------------------------------------------------------------------------------------
  Variables
--------------------------------------------------------------------------------------*/
byte buttonJustPressed  = false;         //this will be true after a ReadButtons() call if triggered
byte buttonJustReleased = false;         //this will be true after a ReadButtons() call if triggered
byte buttonWas          = BUTTON_NONE;   //used by ReadButtons() for detection of button events
/*--------------------------------------------------------------------------------------
  Init the LCD library with the LCD pins to be used
--------------------------------------------------------------------------------------*/
// LiquidCrystal lcd( 8, 13, 9, 4, 5, 6, 7 );   //Pins for the LinkSprie 16x2 LCD shield. LCD: ( RS, E, LCD-D4, LCD-D5, LCD-D6, LCD-D7 )
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);   //Pins for the ADfruit 20x4 LCD display. LCD: ( RS, E, LCD-D4, LCD-D5, LCD-D6, LCD-D7 )
int adc_key_val[5] ={50, 200, 400, 600, 800 };
int NUM_KEYS = 5;
int adc_key_in;
int key=-1;
int oldkey=-1;
int IN=0;
int MIN= 0;       // RAM Extended fully.
int MAX= 4.26;   // RAM Retracted fully.
int POS = 0;
float Volts = 0;
float Vscale = 0.002994;    // calibration factor for voltage input
float MMscale = 0.002336;  //***  V/mm ***//
float INscale = 0.059333;   //** V/inch  **//
int POS_old = 0;        //**  Previouse position in mm **//

int Ram= 050;    //** Initial RAM (Retract) Setpoit in mm. **//
int Wgt= 0340;   //**  Initial Weight (Extend) Setpoint in mm. **//

int ledPin = 13;                 // LED connected to digital pin 13
char Str1[ ] = " ===";   //** Direction:  "==" is not moving; "->" is Deploy; "<--" is Retrieve.  **//
char Str2[ ] = " -->";
char Str3[ ] = " <--";
int DIR= 0;      //** Current Direction of travel (0,1,2)
int pDIR= 0;     //** Previouse direction of travel (0,1,2)
int CycleCnt= 0;  //** Initi Cycle Count to zero after reset. **//

// you can change the overall brightness by range 0 -> 255
int brightness = 255;

/*--------------------------------------------------------------------------------------
  setup()
  Called by the Arduino framework once, before the main loop begins
--------------------------------------------------------------------------------------*/
void setup()
{
     pinMode(ledPin, OUTPUT);      // sets the digital pin as output
     pinMode(REDLITE, OUTPUT);    // backlight
     pinMode(GREENLITE, OUTPUT);  
     pinMode(BLUELITE, OUTPUT);   
     brightness = 100;
     
  //button adc input
   pinMode( BUTTON_ADC_PIN, INPUT );         //ensure A0 is an input
   digitalWrite( BUTTON_ADC_PIN, LOW );      //ensure pullup is off on A0
   
   setBacklight(158, 0, 158);
   
   //set up the LCD number of columns and rows: 
   lcd.begin( 20, 4 );
   //Print some initial text to the LCD.
   lcd.setCursor( 0, 0 );   //top left
   //          12345678901234567890
   lcd.print( "CBOS Stroke Position" );
   //
    lcd.setCursor( 0, 1 );   //bottom left
   //          1234567890123456
   lcd.print( "      Readout" );
    delay(2000);
    lcd.setCursor( 0, 1 );   //top left
   //          12345678901234567890
   lcd.print( "                    " );
   lcd.setCursor( 0, 2 );   //top left
   //          12345678901234567890
   lcd.print( "                    " );
       lcd.setCursor( 0, 3 );   //top left
   //          12345678901234567890
   lcd.print( "                    " );
  ads1115.begin();

}

/*--------------------------------------------------------------------------------------
  loop()
  Arduino main loop
--------------------------------------------------------------------------------------*/
void loop()
{
   byte button;
    
   int16_t results;
   int16_t results1;
      
   //** Average 3 POS measurements
  results = ads1115.readADC_Differential_0_1(); 
  results1=results;          delay( 100 ); 
  results = ads1115.readADC_Differential_0_1(); 
  results1=results1+results;         delay( 100 ); 
  results = ads1115.readADC_Differential_0_1();  
  results1=results1+results;
  results= results1/3;
  
  Volts = results * Vscale;    // Scale input to voltage
  lcd.setCursor(0, 1);
  lcd.print(Volts); lcd.print("V");
  lcd.setCursor(0, 2);
  //  lcd.print(71.91 - (Volts/INscale)); lcd.print("in    ");

    lcd.print(1826.39 - (Volts/MMscale)); lcd.print("mm");
   POS=(1826.39 - (Volts/MMscale));  // convert POS to mm scaling.

  lcd.setCursor(15, 1);
  
  pDIR=DIR;      // save current DIRection to previous DIR.
  
  //** Determine if RAM is moving out or retracting.... Set DIR **//
  if (POS == POS_old){
         DIR=0; 
         setBacklight(255, 0, 0);     //**   No change Position ===  **//
      }
       else if (POS > POS_old)
         DIR=1;             //** Ram is EXTENDING or Deploying cable. **//
       else if (POS < POS_old)
         DIR=2;             //** RAM is RETRACTING, or retrieving cable.   **//
  
  //** Count one Machine Cycle each time the RAM Returns and Extend again ... (Moves from "R" state to "E" state)
  if ((DIR < pDIR) && (pDIR != 0) )
      {
       CycleCnt++;
      }

  if (POS<= Ram) {
         digitalWrite( RAM_DIRECTION_PIN, HIGH );   //**Retracted limit hit ...need to go extend
         lcd.print("E");
         setBacklight(0, 100, 255);
         delay( 150 ); 
        }   
     else if (POS>= Wgt) {
         digitalWrite( RAM_DIRECTION_PIN, LOW );   //**Extent hit, need to retract.
         lcd.print("R");
         setBacklight(255, 0, 255);
         delay( 150 ); 
       }   
   POS_old= POS;

//** Determine and display direction **//
   lcd.setCursor(16, 1);
  switch (DIR)  {
    case 0:
      lcd.print(Str1);
      break;
    case 1:
      lcd.print(Str2);
      break;
    case 2:
      lcd.print(Str3);
      break;
  }
     
   lcd.setCursor( 16, 4 );   // print the number of cycles since reset (three digits only)
   if( CycleCnt <= 9 )
      lcd.print( " " );   //quick trick to right-justify this 2 digit value when it's a single digit
   lcd.print( CycleCnt ); 
   
}


void setBacklight(uint8_t r, uint8_t g, uint8_t b) { 
  // normalize the red LED - its brighter than the rest!  
r = map(r, 0, 255, 0, 100);  
g = map(g, 0, 255, 0, 150);   
r = map(r, 0, 255, 0, brightness);  
g = map(g, 0, 255, 0, brightness);  
b = map(b, 0, 255, 0, brightness);   
// common anode so invert!  
r = map(r, 0, 255, 255, 0);  
g = map(g, 0, 255, 255, 0);  
b = map(b, 0, 255, 255, 0);  
Serial.print("R = "); 
Serial.print(r, DEC);  
Serial.print(" G = "); 
Serial.print(g, DEC);  
Serial.print(" B = "); 
Serial.println(b, DEC);  
analogWrite(REDLITE, r);  
analogWrite(GREENLITE, g);  
analogWrite(BLUELITE, b);
}

