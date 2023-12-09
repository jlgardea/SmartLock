/* 
* Assignment: Final Project
* Description: Keyless entry system. Can set up to 3-5 digit pins
* Buzzer will sound when door is unlocked, LCD Screeen shows menu
* and status of system. Holding reset button down for 2 seconds
* resets system to have no pins.
*
* I acknowledge all content contained herein, excluding 
*   template or example code, is my own original work.
*
* Demo Link: https://youtu.be/Yundt-XGh0U
*/

#include <Servo.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

//servo motor setup
Servo motor;
const int motorpin = 10;

//Button setup
const int button = 2;

//Buzzer setup
const int buzzer = 3;
const double sound = 261.63;

//Keypad setup
const byte rows = 4; //four rows
const byte cols = 4; //three columns
unsigned char keys[rows][cols] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[rows] = {4, 5, 6, 7}; //connect to the row pinouts of the keypad
byte colPins[cols] = {8, 9, 12, 13}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );

//LCD Screen setup
LiquidCrystal_I2C lcd(0x27, 20, 4);

//EEPROM setup
/** the current address in the EEPROM (i.e. which byte we're going to write to next) **/
int k, j = 0;
unsigned char r = 0x0;
char memData;

//Pin setup
unsigned char pinCount = 0x0; //count of the PIN Numbers stored
const int maxPins = 3; //number of pins that can be stored
String pinNumbers[maxPins]= {};
unsigned char pinSize = 5; //Pin must be 5 digits
String enteredPin = ""; //getting user information
unsigned char t, i; //(t): counter for time unlocked; (i): counter for input entered  
char input; // getting char from kepad
static bool validPin, reset = false; //validPin: true if pin is valid; reset: true if button held down for more than 2 seconds.

//screen menu setups
void mainMenu(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("---DOOR IS LOCKED---");
  lcd.setCursor(0,2);
  lcd.print("PRESS # TO UNLOCK");
  lcd.setCursor(0,3);
  lcd.print("PRESS A TO ADD PIN");
}

void enterPinMenu(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("ENTER YOUR PIN#:");
}

void correctPinMenu(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("--DOOR IS UNLOCKED--");
}

void incorrectPinMenu(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("---INCORRECT PIN---");
}

void maxPinMenu(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("--MAX PINS REACHED--");
}

void AddPinMenu(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("ENTER NEW PIN#:");
}

void savedPinMenu(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("PIN NUMBER IS SAVED");
  lcd.setCursor(0,2);
  lcd.print(pinCount);
  lcd.setCursor(1,2);
  lcd.print(" OF ");
  lcd.setCursor(6,2);
  lcd.print(maxPins);
  lcd.setCursor(7,2);
  lcd.print(" PINS SAVED");
}

void resetMenu(){
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("----SYSTEM RESET----");
  lcd.setCursor(0,2);
  lcd.print("   NO PINS SAVED");
}

//task setup
typedef struct task {
  int state;
  unsigned long period;
  unsigned long elapsedTime;
  int (*TickFct)(int);
    
} task;

int delay_gcd;
const unsigned short tasksNum = 3;
task tasks[tasksNum];

//user action concurrent state machine
enum SM1_States {SM1_INIT, SM1_GETMEM, SM1_HOME, SM1_ENTERPIN, SM1_SAVEPIN, SM1_DISPLAY};
int SM1_Tick(int state){
  switch(state){
    case SM1_INIT:
        state = SM1_GETMEM;
      break;
    case SM1_GETMEM:
      if(k >= maxPins){
        state = SM1_HOME;
        enteredPin = "";
      }
      else {
        state = SM1_GETMEM;
        enteredPin = "";
      }
      break;
    case SM1_HOME:
      input = keypad.getKey();
      if(input == '#'){
        state = SM1_ENTERPIN;
        enterPinMenu();
        i = 0;
      }
      else if(input == 'A'){
        state = SM1_SAVEPIN;
        AddPinMenu();
        i = 0;
        EEPROM[pinCount*(pinSize + 1)] = 'V';
      }
      else if(reset == true){
        state = SM1_DISPLAY;
        resetMenu();
        i = 0;
      }
      else{
        state = SM1_HOME;
      }
      break;
    case SM1_ENTERPIN:
      if(i == pinSize){
        if(enteredPin == pinNumbers[0] || enteredPin == pinNumbers[1] || enteredPin == pinNumbers[2]){
          validPin = true;
          correctPinMenu();
          state = SM1_DISPLAY;
        }
        else{
          validPin = false;
          incorrectPinMenu();
          state = SM1_DISPLAY;
        }
        i = 0;
        enteredPin = "";
      }
      else{
        state = SM1_ENTERPIN;
      }
      break;
    case SM1_SAVEPIN:
      if(pinCount == 3){       
        state = SM1_DISPLAY;
        maxPinMenu();
        i = 0;        
      }
      else if(i == pinSize){
        pinNumbers[pinCount] = enteredPin; 
        pinCount++;
        state = SM1_DISPLAY;
        savedPinMenu();
        i = 0;
        enteredPin = "";
      }
      else{
        state = SM1_SAVEPIN;
      }
      break;
    case SM1_DISPLAY:
      if(validPin == true){
        state = SM1_DISPLAY;
      }
      else if(validPin == false){
        if(i > 30){
          mainMenu();
          state = SM1_HOME;
        }
        else{
          state = SM1_DISPLAY;       
        }
      }
      break;
  }
  switch(state){
    case SM1_INIT:
      break;
    case SM1_GETMEM:
      if (k < maxPins){
        if(EEPROM[k*(pinSize + 1)] == 'V'){
          for(j = 0; j < pinSize; j++){
            memData = EEPROM.read((k+1) + 5*k + j);
            enteredPin.concat(memData);
            i++;
          }
          pinNumbers[k] = enteredPin;
          pinCount++;
        }
        k++;
        i = 0;
      }  
      break;
    case SM1_HOME:
      break;
    case SM1_ENTERPIN:
      input = keypad.getKey();
      if(input != NO_KEY && i < pinSize){
        enteredPin.concat(input);
        lcd.setCursor(i,2);
        lcd.print("*");
        i++;
      }
      break;
    case SM1_SAVEPIN:
      input = keypad.getKey();
      if(input != NO_KEY && i < pinSize){
        enteredPin.concat(input);
        EEPROM[(pinCount+1) + 5*pinCount + i] =  input;
        lcd.setCursor(i,2);
        lcd.print("*");
        i++;
      }
      break;
    case SM1_DISPLAY:   
      i++;
      break;
  }
  return state;
}

//motor concurrent state machine
enum SM2_States {SM2_INIT, SM2_LOCKED, SM2_UNLOCKED};
int SM2_Tick(int state){
  switch(state){
    case SM2_INIT:
       state = SM2_LOCKED;
       break;
    case SM2_LOCKED:
       if(validPin == true){
          state = SM2_UNLOCKED;
       }
       else{
          state = SM2_LOCKED;  
       }
       break;
    case SM2_UNLOCKED:
       if(t > 120){
          state = SM2_LOCKED;
          validPin = false;
          t = 0;
       }
       else{
          state = SM2_UNLOCKED;  
       }    
       break;
    default:
       break;
  }
  switch(state){
    case SM2_INIT:
       break;
    case SM2_LOCKED:
       motor.write(180);
       noTone(buzzer);
       break;
    case SM2_UNLOCKED:
       motor.write(0);
       tone(buzzer, sound);
       t++;  
       break;    
  }
  return state;
}


//reset button concurrent state machine
enum SM3_States {SM3_INIT, SM3_STANDBY, SM3_CHECK, SM3_RESET};
int SM3_Tick(int state){
  switch(state){
     case SM3_INIT:
        state = SM3_STANDBY;
        break;
     case SM3_STANDBY:
        if(digitalRead(button) == HIGH && pinCount > 0){
          state = SM3_CHECK;
        }
        else{
          state = SM3_STANDBY;
        }
        break;
     case SM3_CHECK:
        if(digitalRead(button) == HIGH && r < 20){
          state = SM3_CHECK;
        }
        else if(r > 20){
          reset = true;
          state = SM3_RESET;
          r = 0;
        }
        else if(digitalRead(button) == LOW){
          state = SM3_STANDBY;
          r = 0;
        }
        break;
     case SM3_RESET:
      if(r >= maxPins){
        state = SM3_STANDBY;
        reset = false;
        r = 0; 
      }
      else {
        state = SM3_RESET;
      }     
        break;
  }
  switch(state){
     case SM3_INIT:
        break;
     case SM3_STANDBY:
        break;
     case SM3_CHECK:
        if(digitalRead(button) == HIGH)
        r++;
        break;
     case SM3_RESET:
        if (r < maxPins){
           EEPROM.update(r*(pinSize + 1), 0);
           pinNumbers[r] = "";
        }
        r++;
        pinCount = 0;
        break;   
  }
  return state;
}

void setup() {
  //motor setup
  motor.attach(motorpin);
  //motor.write(180);

  //button setup
  pinMode(button, INPUT);

  //lcd setup
  lcd.begin(20, 4);
  lcd.setBacklight(255);
  mainMenu();

  //tasks setup
  unsigned char i = 0;
  tasks[i].state = SM1_INIT;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &SM1_Tick;
  i++;
  tasks[i].state = SM2_INIT;
  tasks[i].period = 50;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &SM2_Tick;
  i++;
  tasks[i].state = SM3_INIT;
  tasks[i].period = 100;
  tasks[i].elapsedTime = 0;
  tasks[i].TickFct = &SM3_Tick;

  delay_gcd = 50; 
  
  Serial.begin(9600);

}

void loop() {

  unsigned char i;
  for (i = 0; i < tasksNum; ++i) {
     if ( (millis() - tasks[i].elapsedTime) >= tasks[i].period) {
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = millis();   
     }
   }
}