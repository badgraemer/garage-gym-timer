/***************************************************
  Timer and stopwatch project
  Using:
  - IR sensor and mini remote from www.adafruit.com
  - 1.2" 7-segment LED display from www.adafruit.com and I2C LED Backpack
  - piezo buzzer from www.adafruit.com
=============
  This is a library for our I2C LED Backpacks

  Designed specifically to work with the Adafruit LED 7-Segment backpacks
  ----> http://www.adafruit.com/products/881
  ----> http://www.adafruit.com/products/880
  ----> http://www.adafruit.com/products/879
  ----> http://www.adafruit.com/products/878

  These displays use I2C to communicate, 2 pins are required to
  interface. There are multiple selectable I2C addresses. For backpacks
  with 2 Address Select pins: 0x70, 0x71, 0x72 or 0x73. For backpacks
  with 3 Address Select pins: 0x70 thru 0x77

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/
//Other REFERENCES
/*
//http://forum.arduino.cc/index.php?topic=318551.0
//https://learn.adafruit.com/using-an-infrared-library/controlling-a-servo-with-ir-1
// https://bennthomsen.wordpress.com/arduino/state-machines-in-embedded-systems/state-table-implementation/stopwatch-example/
/*
 Example 37.2 – Super-basic stopwatch using millis();
 http://tronixstuff.com/tutorials > chapter 37
 John Boxall | CC by-sa-nc
*/
//  http://playground.arduino.cc/Main/CountDownTimer
// http://www.hobbytronics.co.uk/arduino-countdown-timer
// http://forums.adafruit.com/viewtopic.php?f=47&t=44852
/////////////////////////////////////////////////////////////////

// Enable one of these two #includes and comment out the other.
// Conditional #include doesn't work due to Arduino IDE shenanigans.
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
//#include <TinyWireM.h> // Enable this line if using Adafruit Trinket, Gemma, etc.
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"
//IR sensor library
#include "IRLib.h"

//declare matrix
Adafruit_7segment matrix = Adafruit_7segment();

/* States. */
#define MENU 0
#define STOPWATCH 1
#define TIMER 2
#define SETTIMER 3
#define RUN 4
#define PAUSE 5
#define TABATA 6

/* Actions */
#define NACT 0
#define INITSW 1
#define EXIT 2
#define STARTSW 3
#define STOP 4
#define SET 5
#define INITTIMER 6
#define STARTTIMER 7
#define SELECTTIMER 8
#define INITTABATA 9
#define STARTTABATA 10

/*Events*/
//Define all the buttons on the Adafruit mini remote after dumping to serial monitor to capture the hex codes
//tutorial reference: https://learn.adafruit.com/using-an-infrared-library/controlling-a-servo-with-ir-1
#define MY_PROTOCOL NEC
#define VOL_DOWN 0xfd00ff
#define PLAY_BUTTON 0xfd807f
#define VOL_UP 0xfd40bf
#define SETUP_BUTTON 0xfd20df
#define UP_ARROW 0xfda05f
#define STOP_BUTTON 0xfd609f
#define LEFT_ARROW 0xfd10ef
#define SELECT_BUTTON 0xfd906f
#define RIGHT_ARROW 0xfd50af
#define BUTTON_0 0xfd30cf
#define DOWN_ARROW 0xfdb04f
#define BACK_BUTTON 0xfd708f
#define BUTTON_1 0xfd08f7
#define BUTTON_2 0xfd8877
#define BUTTON_3 0xfd48b7
#define BUTTON_4 0xfd28d7
#define BUTTON_5 0xfda857
#define BUTTON_6 0xfd6897
#define BUTTON_7 0xfd18e7
#define BUTTON_8 0xfd9867
#define BUTTON_9 0xfd58a7
//end of button definitions

//state transition table
char transtable[9][7] = /* transtab[Input][CurrentState] => NextState */
{
  //MENU,STOPWATCH,SETTIMER,TIMER,RUN,PAUSE,TABATA
  STOPWATCH, STOPWATCH, SETTIMER, TIMER, RUN, PAUSE, TABATA, // BUTTON_1
  SETTIMER, STOPWATCH, SETTIMER, TIMER, RUN, PAUSE, TABATA, // BUTTON_2
  MENU, RUN, SETTIMER, RUN, RUN, RUN, RUN, // PLAY_BUTTON
  MENU, MENU, MENU, SETTIMER, RUN, MENU, MENU, // BACK_BUTTON
  MENU, STOPWATCH, SETTIMER, TIMER, RUN, PAUSE, TABATA, // UP_ARROW
  MENU, STOPWATCH, SETTIMER, TIMER, RUN, PAUSE, TABATA, // DOWN_ARROW
  MENU, STOPWATCH, SETTIMER, TIMER, PAUSE, PAUSE, TABATA, // STOP_BUTTON
  MENU, STOPWATCH, TIMER, TIMER, RUN, PAUSE, TABATA, // SELECT_BUTTON
  TABATA, STOPWATCH, SETTIMER, TIMER, RUN, PAUSE, TABATA, // BUTTON_3
};

//state action table
char actiontable[9][7] = /* transtab[Input][CurrentState] => NextState */
{
  //MENU,STOPWATCH,SETTIMER,TIMER,RUN,PAUSE,TABATA
  INITSW, NACT, NACT, NACT, NACT, NACT, NACT, // BUTTON_1
  INITTIMER, NACT, NACT, NACT, NACT, NACT, NACT, // BUTTON_2
  NACT, STARTSW, NACT, STARTTIMER, NACT, NACT, STARTTABATA, // PLAY_BUTTON
  NACT, EXIT, EXIT, INITTIMER, NACT, EXIT, EXIT, // BACK_BUTTON
  NACT, NACT, SET, NACT, NACT, NACT, NACT, // UP_ARROW
  NACT, NACT, SET, NACT, NACT, NACT, NACT, // DOWN_ARROW
  NACT, NACT, NACT, NACT, STOP, NACT, NACT, // STOP_BUTTON
  NACT, NACT, SELECTTIMER, NACT, NACT, NACT, NACT, // SELECT_BUTTON
  INITTABATA, NACT, NACT, NACT, NACT, NACT, NACT, //BUTTON_3
};

//functions - don't know if this is necessary, not a C expert
void InitAction(void);
void ExitAction(void);
void StartAction();
void HoldAction(void);
void ContAction(void);
void StopAction();
void WriteMenu();
void WriteZeros();
void StartStopwatch();
void DisplayStopwatch();
boolean TimeHasChanged();
boolean TimerHasChanged();
void DisplayElapsedTime();
char PollIRReceiver(void);
char UpdateState();

IRrecv My_Receiver(11);//Receive on pin 11
//IRdecode My_Decoder; //using the next line to specify NEC, per the adafruit tutorial
IRdecodeNEC My_Decoder;

//buzzer pin (TODO)
int speakerPin = 12;
int SolenoidPin = 10;

// other global variables
volatile char button = 0;
char currentstate = MENU; //initial state
long Previous;   // Stores previous code to handle NEC repeat codes
unsigned long startTime, elapsedTime, finishTime, timerStart, timerElapsed, timerInverse;
volatile boolean timeFlag = false;
volatile boolean timerFlag = false;
unsigned long start_num = 0; // Number to countdown from - 3,2,1
unsigned long timer_num = 0; //timer number to countdown from
unsigned long time;
int tabataones = 0; //ones place of tabata counter to keep track of how many cycles
float tabatatens = 0; //tens place
volatile boolean blink = false;
int dingState = LOW;
long previousMillis = 0;
long interval = 250; //colon blink interval
long previousMillis2 = 0;
long dingInterval = 250; //time between dings
long shortdingInterval = 20; //time between dings

// setup loop - run once
void setup() {
  //Serial.begin(9600);
  pinMode(SolenoidPin, OUTPUT);
  matrix.begin(0x70);
  matrix.setBrightness(5);   //brightness goes from 0-15
  My_Receiver.enableIRIn(); // Start the receiver
  WriteMenu();
}

void loop()
{
  if (PollIRReceiver() ) //Poll receiver
  {
    switch (actiontable[button][currentstate])
    { // Call state transition action
      case NACT: ;
        break;
      case INITSW: InitSWAction();
        break;
      case EXIT: ExitAction();
        break;
      case STARTSW: StartSWAction();
        break;
      case SET: SetAction();
        break;
      case STOP: StopAction();
        break;
      case INITTIMER: InitTimerAction();
        break;
      case STARTTIMER: StartTimerAction();
        break;
      case SELECTTIMER: SelectTimerAction();
        break;
      case INITTABATA: InitTabataAction();
        break;
      case STARTTABATA: StartTabataAction();
        break;
      default: ;
        break;
    }
    currentstate = transtable[button][currentstate]; //Update state
  }
}

/////////////////
/*
 * Start of all the state action functions
 */
/////////////////

void InitSWAction(void)
{
  //Serial.println("init sw action");
  matrix.writeDigitRaw(0, B01010100);
  matrix.writeDigitRaw(1, B01010100);
  //matrix.writeDigitNum(1, 0);
  matrix.drawColon(false);
  matrix.writeDigitNum(3, 0);
  matrix.writeDigitNum(4, 1);
  //matrix.blinkRate(2);
  matrix.writeDisplay();
}

void InitTimerAction(void)
{
 // Serial.println("init timer action");
  matrix.writeDigitRaw(0, B01010100);
  matrix.writeDigitRaw(1, B01010100);
  //matrix.writeDigitNum(1, 0);
  matrix.drawColon(false);
  matrix.writeDigitNum(3, 0);
  matrix.writeDigitNum(4, 2);
  //matrix.blinkRate(2);
  matrix.writeDisplay();
}

void InitTabataAction(void)
{
  //Serial.println("init tabata action");
  matrix.writeDigitRaw(0, B01010100);
  matrix.writeDigitRaw(1, B01010100);
  //matrix.writeDigitNum(1, 0);
  matrix.drawColon(false);
  matrix.writeDigitNum(3, 0);
  matrix.writeDigitNum(4, 3);
  //matrix.blinkRate(2);
  matrix.writeDisplay();
}

void ExitAction(void)
{
  //Serial.println("exit action");
  //Serial.println('currentstate');
  timer_num = 0;
  WriteMenu();
}

void StartSWAction()
{
 // Serial.println("start stopwatch action");
  ThreeTwoOne();
  StartStopwatch();

  while (TimeHasChanged() ) {
    DisplayStopwatch();
    Blink_Colon();
    if (PollIRReceiver() ) {
      if (button == 6) {
        StopAction();
        currentstate = transtable[button][currentstate]; //Update state
        return;
      }
    }
  }
}

void StartTimerAction()
{
  //Serial.println("start timer action");
  ThreeTwoOne(); // 3,2,1 countdown
  start_num = (timer_num * 60) + 1; // assign the toggled timer_num to start_num to begin countdown
  //Serial.println(start_num);
  StartTimer();  // start the timer
  while (TimerHasChanged() ) {
    DisplayTimerTime();
    Blink_Colon();
    if (PollIRReceiver() ) {
      if (button == 6) {
        StopAction();
        currentstate = transtable[button][currentstate]; //Update state
        return;
      }
    }
  }
  StopAction();
  DoubleDing();
  DoubleDing();
  DoubleDing();
  button = 6;
  currentstate = PAUSE;
  currentstate = transtable[button][currentstate]; //Update state
  return;
}

void StartTabataAction()
{
  //Serial.println("start tabata action");
  ThreeTwoOne();
  for (int tabatat = 1; tabatat < 100; tabatat++) {
    tabataones = (tabatat % 10);
    tabatatens = int(tabatat / 10);

    start_num = 21; // 20 second cycle
    StartTimer();  // start the timer

    while (TimerHasChanged() ) {
      DisplayTabataTime();
      Blink_Colon();
      if (PollIRReceiver() ) {
        if (button == 6) {
          StopAction();
          currentstate = transtable[button][currentstate]; //Update state
          return;
        }
      }
    }
    SingleDing();

    start_num = 11; // 10 second cycle
    StartTimer();  // start the timer

    while (TimerHasChanged() ) {
      DisplayTabataTime();
      Blink_Colon();
      if (PollIRReceiver() ) {
        if (button == 6) {
          StopAction();
          currentstate = transtable[button][currentstate]; //Update state
          return;
        }
      }
    }
    DoubleDing();
  }
}

void SelectTimerAction()
{
 // Serial.println("select timer action");
 // Serial.println(timer_num);
  matrix.writeDigitRaw(0, 0x00);
  matrix.writeDigitNum(1, 5);
  matrix.drawColon(false);
  matrix.writeDigitNum(3, 0xE);
  matrix.writeDigitRaw(4, B00110001);
  //matrix.blinkRate(0);
  matrix.writeDisplay();  // write the word "5ET" to indicate that the timer is set
  return;
}

void SetAction(void)
{
 int timerOnes = 0;
 int timerTens = 0;
 // Serial.println("set action");
  if (button == 4) {
    if (timer_num < 59) {
      timer_num = timer_num + 1;
      timerOnes = timer_num % 10;
      timerTens = timer_num / 10;
     // Serial.println(timer_num);
      matrix.writeDigitRaw(0, 0x00);
      matrix.writeDigitRaw(1, 0x00);
      matrix.drawColon(false);
      matrix.writeDigitNum(3, timerTens, DEC);
      matrix.writeDigitNum(4, timerOnes, DEC);
    }
    else timer_num = timer_num;
  }
  else if (button == 5) {
    if (timer_num > 0) {
      timer_num = timer_num - 1;
      timerOnes = timer_num % 10;
      timerTens = timer_num / 10;
      //Serial.println(timer_num);
      matrix.writeDigitRaw(0, 0x00);
      matrix.writeDigitRaw(1, 0x00);
      matrix.drawColon(false);
      matrix.writeDigitNum(3, timerTens, DEC);
      matrix.writeDigitNum(4, timerOnes, DEC);
    }
    else timer_num = timer_num;
  }
  currentstate = transtable[button][currentstate]; //Update state
  //matrix.blinkRate(2);
  matrix.writeDisplay();
  return;
}

void StopAction(void)
{
  timeFlag = false;
 // Serial.println("stop action");
  //Serial.println('currentstate');
}

//function to write an "m" for "menu" on the display
void WriteMenu() {
  matrix.writeDigitRaw(0, B01010100);
  matrix.writeDigitRaw(1, B01010100);
  matrix.drawColon(false);
  matrix.writeDigitNum(3, 0);
  matrix.writeDigitNum(4, 0);
  //matrix.blinkRate(2);
  matrix.writeDisplay();
}

//function to write all zeros to the display
void WriteZeros() {
  matrix.writeDigitNum(1, 0);
  matrix.writeDigitNum(2, 0);
  matrix.drawColon(true);
  matrix.writeDigitNum(3, 0);
  matrix.writeDigitNum(4, 0);
  //matrix.blinkRate(2);
  matrix.writeDisplay();
}

//function to start stopwatch
void StartStopwatch() {
  startTime = millis();
  timeFlag = true;
}

//function to start timer
void StartTimer() {
  timerStart = millis();
  timerFlag = true;
}

//function to display running stopwatch time
void DisplayStopwatch() {
  float h, m, s, ms;
  unsigned long over;
  elapsedTime =   millis() - startTime;  //store elapsed time
  h = int(elapsedTime / 3600000);
  over = elapsedTime % 3600000;
  m = int(over / 60000);
  over = over % 60000;
  s = int(over / 1000);
  ms = over % 1000;
  /*Serial.print("Elapsed time: ");
  Serial.print(h, 0);
  Serial.print("h ");
  Serial.print(m, 0);
  Serial.print("m ");
  Serial.print(s, 0);
  Serial.print("s ");
  Serial.print(ms, 0);
  Serial.println("ms");
  Serial.println(); */

  matrix.writeDigitNum(0, m / 10);
  matrix.writeDigitNum(1, int(m) % 10);
  //matrix.drawColon(true);
  matrix.writeDigitNum(3, s / 10);
  matrix.writeDigitNum(4, int(s) % 10);
  matrix.writeDisplay();
  //Blink_Colon();
}

//function to display running timer time
void DisplayTimerTime() {
  float h, m, s, ms;
  unsigned long over;
  timerElapsed = millis() - timerStart;  //store elapsed time
  timerInverse = ((start_num * 1000) - timerElapsed);
  if (timerInverse > 900) {
    timerFlag = true;
  }
  else timerFlag = false;
  h = int(timerInverse / 3600000);
  over = timerInverse % 3600000;
  m = int(over / 60000);
  over = over % 60000;
  s = int(over / 1000);
  ms = over % 1000;
  /*Serial.print("Elapsed time: ");
  Serial.print(h, 0);
  Serial.print("h ");
  Serial.print(m, 0);
  Serial.print("m ");
  Serial.print(s, 0);
  Serial.print("s ");
  Serial.print(ms, 0);
  Serial.println("ms");
  Serial.println(); */

  matrix.writeDigitNum(0, m / 10);
  matrix.writeDigitNum(1, int(m) % 10);
  //matrix.drawColon(true);
  matrix.writeDigitNum(3, s / 10);
  matrix.writeDigitNum(4, int(s) % 10);
  matrix.writeDisplay();
  //Blink_Colon();
}

//function to display running tabata time
void DisplayTabataTime() {
  float h, m, s, ms;
  unsigned long over;
  timerElapsed = millis() - timerStart;  //store elapsed time
  timerInverse = ((start_num * 1000) - timerElapsed);
  if (timerInverse > 900) {
    timerFlag = true;
  }
  else timerFlag = false;
  h = int(timerInverse / 3600000);
  over = timerInverse % 3600000;
  m = int(over / 60000);
  over = over % 60000;
  s = int(over / 1000);
  ms = over % 1000;

  matrix.writeDigitNum(0, tabatatens);
  matrix.writeDigitNum(1, tabataones);
  //matrix.drawColon(true);
  matrix.writeDigitNum(3, s / 10);
  matrix.writeDigitNum(4, int(s) % 10);
  matrix.writeDisplay();
  //Blink_Colon();
}

//function to display elapsed stopwatch time
void DisplayElapsedTime() {
  float h, m, s, ms;
  unsigned long over;
  h = int((finishTime - startTime) / 3600000);
  over = (finishTime - startTime) % 3600000;
  m = int(over / 60000);
  over = over % 60000;
  s = int(over / 1000);
  ms = over % 1000;

  matrix.writeDigitNum(0, m / 10);
  matrix.writeDigitNum(1, int(m) % 10);
  matrix.drawColon(true);
  matrix.writeDigitNum(3, s / 10);
  matrix.writeDigitNum(4, int(s) % 10);
  matrix.writeDisplay();
}

//check the time flag
boolean TimeHasChanged()
{
  return timeFlag;
}

//check the timer flag
boolean TimerHasChanged()
{
  return timerFlag;
}

char PollIRReceiver() {
  if (My_Receiver.GetResults(&My_Decoder)) {
    My_Decoder.decode();
    if (My_Decoder.decode_type == MY_PROTOCOL) {
      if (My_Decoder.value == 0xFFFFFFFF) //handles NEC repeat codes - holding down buttons
        My_Decoder.value = Previous;

      switch (My_Decoder.value) {
        case BUTTON_1: button = 0;
          break;
        case BUTTON_2: button = 1;
          break;
        case PLAY_BUTTON: button = 2;
          break;
        case BACK_BUTTON: button = 3;
          break;
        case UP_ARROW: button = 4;
          break;
        case DOWN_ARROW: button = 5;
          break;
        case STOP_BUTTON: button = 6;
          break;
        case SELECT_BUTTON: button = 7;
          break;
        case BUTTON_3: button = 8;
          break;
        default: ;
          break;
      }
    }
    Previous = My_Decoder.value;
    My_Receiver.resume();
    return 1;
  }
  else return 0;
}
//double ding
void DoubleDing() {
 // Serial.println("ding ding");
  digitalWrite(SolenoidPin, HIGH);
  delay(shortdingInterval);
  digitalWrite(SolenoidPin, LOW);
  delay(dingInterval);
  digitalWrite(SolenoidPin, HIGH);
  delay(shortdingInterval);
  digitalWrite(SolenoidPin, LOW);
  delay(dingInterval);
 // TODO refactor into ding without delay
  /*unsigned long currentMillis2 = millis();
  int counter = 0;

  if ((currentMillis2 - previousMillis2 > dingInterval)){
    previousMillis2 = currentMillis2;

    if (dingState == LOW) 
      dingState = HIGH;
    else
      dingState = LOW;

      digitalWrite(SolenoidPin, dingState);

      counter++;
  }
    if (counter <4){
    digitalWrite(SolenoidPin, dingState);
  }
  else
    digitalWrite(SolenoidPin, LOW); */
  
}

//single ding
void SingleDing() {
 // Serial.println("ding");
  digitalWrite(SolenoidPin, HIGH);
  delay(shortdingInterval);
  digitalWrite(SolenoidPin, LOW);
  delay(dingInterval);
}

//321-go countdown
void ThreeTwoOne() {
  start_num = 4; // start at 4 - extra second needed for some reason.

  StartTimer();
  while (TimerHasChanged() ) {
    DisplayTimerTime();
    //Blink_Colon();
  }
  DoubleDing();
  return;
}

void Blink_Colon () { // "blink without delay" function to blink the colon while clock is running

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > interval) {
    // save the last time you blinked
    previousMillis = currentMillis;

    // if the colon is off turn it on and vice-versa:
    if (blink == false) {
      matrix.writeDigitRaw (2, 0x02);
      matrix.writeDisplay();
      blink = true;
    }
    else {
      matrix.writeDigitRaw (2, 0x00);
      matrix.writeDisplay();
      blink = false;
    }
  }
}
