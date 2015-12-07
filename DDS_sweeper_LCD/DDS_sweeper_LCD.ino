/*
  _____  _____   _____    _____                                  
 |  __ \|  __ \ / ____|  / ____|                                 
 | |  | | |  | | (___   | (_____      _____  ___ _ __   ___ _ __ 
 | |  | | |  | |\___ \   \___ \ \ /\ / / _ \/ _ \ '_ \ / _ \ '__|
 | |__| | |__| |____) |  ____) \ V  V /  __/  __/ |_) |  __/ |   
 |_____/|_____/|_____/  |_____/ \_/\_/ \___|\___| .__/ \___|_|   
                                                | |              
                                                |_|              

/***************************************************************************\
*  Name    : DDS_Sweeper.BAS                                                *
*  Author  : Beric Dunn (K6BEZ)                                             *
*  Notice  : Copyright (c) 2013  CC-BY-SA                                   *
*          : Creative Commons Attribution-ShareAlike 3.0 Unported License   *
*  Date    : 9/26/2013                                                      *
*  Version : 1.0                                                            *
*  Notes   : Written using for the Arduino Micro                            *
*          :   Pins:                                                        *
*          :    A0 - Reverse Detector Analog in                             *
*          :    A1 - Forward Detector Analog in                             *
*                                                                           *
*  LCD     : Mark Phillips, NI2O  20141202                                  *
*                                                                           *
*  Modifications done by KK6ANP 7/12/2015                                   *
*  For more info and the PC program please go to http://lulzsoft.com        *
*  Or https://github.com/dlichterman/kk6anp_sweep                           *  
*                                                                           *
*  Note to self - pick arduino uno to flash                                 *
*                                                                           *
\***************************************************************************/

//#include <MenuBackend.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Define Pins used to control AD9850 DDS
#define FQ_UD 10
#define SDAT 11
#define SCLK 9
#define RESET 12
#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }


// Define pins used for the input control
const int buttonEsc = A2;
const int buttonUp = A3;
const int buttonDown = A4;
const int buttonEnter = A5;
int lastButtonPushed = 0;

int lastButtonEnterState = LOW;   // the previous reading from the Enter input pin
int lastButtonEscState = LOW;   // the previous reading from the Esc input pin
int lastButtonLeftState = LOW;   // the previous reading from the Left input pin
int lastButtonRightState = LOW;   // the previous reading from the Right input pin

long lastEnterDebounceTime = 0;  // the last time the output pin was toggled
long lastEscDebounceTime = 0;  // the last time the output pin was toggled
long lastLeftDebounceTime = 0;  // the last time the output pin was toggled
long lastRightDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 500;    // the debounce time

/* //Menu variables
MenuBackend menu = MenuBackend(menuUsed,menuChanged);
//initialize menuitems
    MenuItem menu1Item1 = MenuItem("Item1");
      MenuItem menuItem1SubItem1 = MenuItem("Item1SubItem1");
      MenuItem menuItem1SubItem2 = MenuItem("Item1SubItem2");
    MenuItem menu1Item2 = MenuItem("Item2");
      MenuItem menuItem2SubItem1 = MenuItem("Item2SubItem1");
      MenuItem menuItem2SubItem2 = MenuItem("Item2SubItem2");
      MenuItem menuItem3SubItem3 = MenuItem("Item2SubItem3");
    MenuItem menu1Item3 = MenuItem("Item3");
*/

double Fstart_MHz = 1;  // Start Frequency for sweep
double Fstop_MHz = 10;  // Stop Frequency for sweep
double current_freq_MHz; // Temp variable used during sweep
long serial_input_number; // Used to build number from serial stream
int num_steps = 100; // Number of steps to use in the sweep
char incoming_char; // Character read from serial stream

// Define LCD pins
LiquidCrystal lcd(7,6,5,4,3,2); // LCD keypad shield from ebay

// the setup routine runs once when you press reset:
void setup() {
  // Configiure DDS control pins for digital output
  pinMode(FQ_UD,OUTPUT);
  pinMode(SCLK,OUTPUT);
  pinMode(SDAT,OUTPUT);
  pinMode(RESET,OUTPUT);
  pulseHigh(RESET);
  pulseHigh(SCLK);
  pulseHigh(FQ_UD);  // this pulse enables serial mode - Datasheet page 12 figure 10
  
  
  // Configure A2-A4 as digital inputs for use with the buttons
  pinMode(A2,INPUT_PULLUP);
  pinMode(A3,INPUT_PULLUP);
  pinMode(A4,INPUT_PULLUP);
  pinMode(A5,INPUT_PULLUP);
  
  // Configure LED pin for digital output
  pinMode(13,OUTPUT);

  // Set up analog inputs on A0 and A1, internal reference voltage, to sense the SWR etc
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  analogReference(EXTERNAL);
  
  // initialize serial communication at 57600 baud
  Serial.begin(57600);
  // Send some brag stuff to the console
  Serial.println("K6BEZ Arduino Antenna Analyser");
  Serial.println();
 
  // Start the LCD
  lcd.begin(16,2);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Testing ...");
 
  // generate 3 frequencies delayed by 1 second each - proves the generator works.
  Serial.println ("Testing 2.5MHz ...");
  SetDDSFreq(2500000); // 2.5MHz
  delay(1000);
  Serial.println ("Testing 5MHz ...");
  SetDDSFreq(5000000); // 5MHz
  delay(1000);
  Serial.println ("Testing 10MHz ...");
  SetDDSFreq(10000000);// 10MHz 
  delay(1000);
  Serial.println ("Reseting the DDS generator ...");
  pulseHigh(RESET); // turn the generator off
 
  // write some brag stuff to the LCD
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" K6BEZ Antenna");
  lcd.setCursor(0,1);
  lcd.print(" Analyser V1.1");
  delay(3000);
  Serial.println ("Lets Go!!");
  

/* //configure menu
  menu.getRoot().add(menu1Item1);
  menu1Item1.addRight(menu1Item2).addRight(menu1Item3);
  menu1Item1.add(menuItem1SubItem1).addRight(menuItem1SubItem2);
  menu1Item2.add(menuItem2SubItem1).addRight(menuItem2SubItem2).addRight(menuItem3SubItem3);
  menu.toRoot();
  lcd.setCursor(0,0);  
  lcd.print("www.coagula.org");
*/
//}  

 
  
  //Initialise the incoming serial number to zero
  serial_input_number=0;

}

// the loop routine runs over and over again forever:
void loop() {
  //Check for character
  if(Serial.available()>0){
    incoming_char = Serial.read();
    switch(incoming_char){
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      serial_input_number=serial_input_number*10+(incoming_char-'0');
      break;
    case 'A':
      //Turn frequency into FStart
      Fstart_MHz = ((double)serial_input_number)/1000000;
      serial_input_number=0;
      break;
    case 'B':
      //Turn frequency into FStart
      Fstop_MHz = ((double)serial_input_number)/1000000;
      serial_input_number=0;
      break;
    case 'C':
      //Turn frequency into FStart and set DDS output to single frequency
      Fstart_MHz = ((double)serial_input_number)/1000000;
      SetDDSFreq(Fstart_MHz);
      serial_input_number=0;    
      break;
    case 'N':
      // Set number of steps in the sweep
      num_steps = serial_input_number;
      serial_input_number=0;
      break;
    case 'R':
      asm volatile ("  jmp 0");   
      break;
    case 'S':    
    case 's':    
      Perform_sweep();
      break;
    case '?':
      // Report current configuration to PC   
     lcd.clear(); 
     lcd.setCursor(0,0);
      Serial.print("Start Freq:");
      lcd.print("Start Freq: ");
      Serial.println(Fstart_MHz*1000000);
      lcd.print(Fstart_MHz);
      lcd.setCursor(0,1);
      Serial.print("Stop Freq:");
      lcd.print("Stop Freq : ");
      Serial.println(Fstop_MHz*1000000);
      lcd.print(Fstop_MHz);
      Serial.print("Num Steps:");
      Serial.println(num_steps);
      break;
    }
    Serial.flush();     
  } 
}

void Perform_sweep(){
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("Sweeping");
  double FWD=0;
  double REV=0;
  double VSWR;
  double Fstep_MHz = (Fstop_MHz-Fstart_MHz)/num_steps;
 
  // Start loop 
  SetDDSFreq(Fstart_MHz);
  for(int i=0;i<=num_steps;i++){
    // Calculate current frequency
    current_freq_MHz = Fstart_MHz + i*Fstep_MHz;
    // Set DDS to current frequency
    SetDDSFreq(current_freq_MHz*1000000);
    // Wait a little for settling
    delay(10);
    // Read the forawrd and reverse voltages
    REV = analogRead(A0);
    FWD = analogRead(A1);
    if(REV>=FWD){
      // To avoid a divide by zero or negative VSWR then set to max 999
      VSWR = 999;
    }
    else{
      // Calculate VSWR
      VSWR = (FWD+REV)/(FWD-REV);
    }
    
    // Send current line back to PC over serial bus
    /*Serial.print(current_freq_MHz*1000000);
    Serial.print(",0,");
    Serial.print(int(VSWR*1000));
    Serial.print(",");
    Serial.print(FWD);
    Serial.print(",");
    Serial.println(REV);*/
    // Send current line back to PC over serial bus
    Serial.print(current_freq_MHz*1000000);
    Serial.print(",");
    Serial.println(int32_t(VSWR*1000));
    delay(20);
  }
  // Send "End" to PC to indicate end of sweep and turn off the generator
  Serial.println("End");
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("Finished");
  Serial.flush(); 
  pulseHigh(RESET);  
}

void SetDDSFreq(double Freq_Hz){
  // Calculate the DDS word - from AD9850 Datasheet
  int32_t f = Freq_Hz * 4294967295/125000000;
  // Send one byte at a time
  for (int b=0;b<4;b++,f>>=8){
    send_byte(f & 0xFF);
  }
  // 5th byte needs to be zeros
  send_byte(0);
  // Strobe the Update pin to tell DDS to use values
 pulseHigh(FQ_UD);
}

void send_byte(byte data_to_send){
  // Bit bang the byte over the SPI bus
  for (int i=0; i<8; i++,data_to_send>>=1){
    // Set Data bit on output pin
    digitalWrite(SDAT,data_to_send & 0x01);
    // Strobe the clock pin
    pulseHigh(SCLK);
    }
}
