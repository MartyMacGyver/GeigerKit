/* GeigerKit_Default sketch     SHIPPING    21796 / 877 bytes in 1.0.5               bHogan 6/6/14
 * This sketch was written for the DIYGeigerCounter Kit sold here:
 *      https://sites.google.com/site/diygeigercounter/home
 * DIY Geiger invests a lot time and resources in providing this open source code. Please support it 
 * by considering what knock-off versions of the hardware really are.
 * It requires the Arduino IDE rel. 1.0.0 or above to compile.
 *
 * FEATURES:
 * The features in this release are discribed on the DIYGeigerCounter web site:
 * http://sites.google.com/site/diygeigercounter/software-features
 * NEW THIS VERSION:
 * - Uses NEC IR protocol for supplied remote. Sony or Phillips supported via #define
 * 
 * SETUP: See GeigerKit.h for pin maping
 * TODO: - CHECK SRAM!
 *
 * THIS PROGRAM AND IT'S MEASUREMENTS IS NOT INTENDED TO GUIDE ACTIONS TO TAKE, OR NOT
 * TO TAKE, REGARDING EXPOSURE TO RADIATION. THE GEIGER KIT AND IT'S SOFTWARE ARE FOR
 * EDUCATIONAL PURPOSES ONLY. DO NOT RELY ON THEM IN HAZARDOUS SITUATIONS!
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * Do not remove information from this header.
 */

//----------------------------------------------------------------------------------------------+
//                              User setup #defines
//----------------------------------------------------------------------------------------------+
#define IR_SONY        false            // use Sony IR protocol instead of NEC 
#define IR_RC5         false            // use Phillips RC5 IR protocol instead of NEC 
#define EIGHT_CHAR     false            // formats for 2x8 LCD when true
#define ANDROID        false            // include Android support if true
#define TONE_POT_ADJ   false            // if true, use a pot attached to A0 to adjust tone instead of menu
//////////////////////////// THESE DEFINES HAVE PRECOMPILER ISSUES ! ///////////////////////////
#define ANALOG_METER   false            // if true, support for analog meter output - REQUIRES HARDWARE - see site
/////////////////////////////////  YOU MUST ALSO UNCOMMENT #include <Wire.h> IF TRUE
#define DOGM_LCD       false            // if true, DogM LCD used for display (SPI interface)
///////////////////////////////    YOU MUST ALSO UNCOMMENT DogLcd lcd(...) IF TRUE
//----------------------------------------------------------------------------------------------+
//                       End user setup #defines (others in GeigerKit.h)
//----------------------------------------------------------------------------------------------+

#include <Arduino.h>
#include <EEPROM.h>
#include <PinChangeInt.h>
#include "GeigerKit.h"
#include "IR.h"

#if (ANDROID)
#include <MeetAndroid.h>                // connects Geiger to SensorGraph on Android
#endif

#if (DOGM_LCD)
#include <DogLcd.h>                     // wayoda's library - http://code.google.com/p/doglcd/
#else
#include <LiquidCrystal.h>              // HD44780 compatible LCDs work with this lib
#endif

#if (ANALOG_METER)                      // This construct doesn't work! Wire.h will be included no matter what
//#include <Wire.h>                     // MUST COMMENT OUT IF NOT USED - else 1226 bytes added
#endif


#if (DOGM_LCD) // instantiate the DogM with pins for (SI, CLK, RS, CSB, [Reset, Backlight])
//DogLcd lcd(DOGM_SI, DOGM_CLK, DOGM_RS, DOGM_CSB, DOGM_RST, DOGM_BKLT); // UNCOMMENT IF USED
#else // instantiate the LiquidCrystal with for (RS, Enable, D4, D5, D6, D7)
LiquidCrystal lcd(LCDPIN_RS, LCDPIN_EN, LCDPIN_D4,LCDPIN_D5, LCDPIN_D6, LCDPIN_D7);
#endif

#if (ANDROID)
MeetAndroid Android;
#endif

#if IR_SONY && IR_RC5
  #error "Only one alternate remote type can be defined!"
#endif

//----------------------------------------------------------------------------------------------+
//                                     DEBUG Defines
//----------------------------------------------------------------------------------------------+
#define DEBUG          false            // if true, shows available memory and other debug info
#define SELF_TEST      false            // ROUGH simulation of a 360CPM count - not handled like an INT so ~348CPM 

//----------------------------------------------------------------------------------------------+
//                                      Functions
//----------------------------------------------------------------------------------------------+

void setup(){
  float globalBgRadAvg;
  Serial.begin(9600);                   // comspec 96,N,8,1
#if (ANALOG_METER)
  Wire.begin();                         // for PCF8591 I2C chip for meter output
#endif  
  attachInterrupt(0,GetEvent,FALLING);  // Geiger event on pin 2 triggers interrupt
  pinMode(LED_PIN,OUTPUT);              // setup LED pin
  pinMode(TUBE_SEL,INPUT);              // setup tube select jumper pin
  pinMode(SEL_BUTTON,INPUT);            // setup menu button
  pinMode(ALARM_PIN, OUTPUT);           // setup Alarm pin
  digitalWrite(TUBE_SEL, HIGH);         // set 20K pullup on jumper pins(low active)
  digitalWrite(SEL_BUTTON, HIGH);
  pinMode(NULL_BUTTON,INPUT);           // null point set button
  digitalWrite(NULL_BUTTON, HIGH);      // turn on the internal pullup
#if (TONE_POT_ADJ)
  pinMode(TONE_POT,INPUT);
#endif
  pinMode(IR_PIN,INPUT);                // setup IR Input pin
  digitalWrite(IR_PIN, HIGH);

  PCintPort::attachInterrupt(IR_PIN, &IR_ISR, FALLING);  // add more attachInterrupt code as required
  Blink(LED_PIN,4);                     // show it's alive

  // Reset everything if the select button is held down during power-on
  if(readButton(SEL_BUTTON)== LOW) {
    clearDisp();                        // clear the screen
    lcd.print(F("RESET EEPROM!"));      // show it reset
    if(readButton(SEL_BUTTON)==LOW) resetToFactoryDefaults();            // Reset all the settings if the button is still held down
    delay(3000);                        // leave it up a bit
  }

#if (DOGM_LCD)
  lcd.begin(DOG_LCD_M162,DOGM_CONTRAST,DOG_LCD_VCC_5V); // for 3.3V use DOG_LCD_VCC_3V
  lcd.createChar(0, bar_0);             // load 7 custom characters in the LCD
  lcd.createChar(1, bar_1);
  lcd.createChar(2, bar_2);
  lcd.createChar(3, bar_3);
  lcd.createChar(4, bar_4);
  lcd.createChar(5, bar_5);
  lcd.home();
  lcd.setBacklight(DOGM_BKL_HIGH,true); // set initial brightness, true = PWM control
  lcd.noCursor();                       // DOGM cursor is on by default - turn it off
#elif (EIGHT_CHAR)
  lcd.begin(8,2);                       // cols, rows of display (8x2, 16x2, etc.)
  lcd.createChar(0, cpmIcon);           // since there is no bar graph we can create special
  lcd.createChar(1, usvIcon);           //   chars for uSv, CPM, and the 1 & 10 minute icons
  lcd.createChar(2, oneIcon);
  lcd.createChar(3, tenIcon);
  lcd.createChar(4, cpm2Icon);
#else
  lcd.begin(16,2);                      // cols, rows of display (8x2, 16x2, etc.)
  lcd.createChar(0, bar_0);             // load 7 custom characters in the LCD
  lcd.createChar(1, bar_1);
  lcd.createChar(2, bar_2);
  lcd.createChar(3, bar_3);
  lcd.createChar(4, bar_4);
  lcd.createChar(5, bar_5);
#endif

  Get_Settings();

#if (ANDROID)
  Android.registerFunction(androidInput,'o'); // calls androidInput() when new input frpm droid
#endif

  clearDisp();                          // clear the screen
#if (EIGHT_CHAR)
  lcd.print(F("GEIGER!"));              // display a simple banner
  lcd.setCursor(0,1);                   // set cursor on line 2
  lcd.print(F(" v10.3"));               // display the version
#else
  lcd.print(F("   GEIGER KIT"));        // display a simple banner
  lcd.setCursor(0,1);                   // set cursor on line 2
  lcd.print(F("   Ver. 11.0"));        // display the version
#endif
  delay (1500);                         // leave the banner up for a bit
  clearDisp();                          // clear the screen

#if (EIGHT_CHAR)
  lcd.print(doseRatio,0);               // display conversion ratio in use 
  lcd.print(F(" Rate"));
  lcd.setCursor(0,1);                   // set cursor on line 2
  lcd.print(readVcc()/1000. ,2);        // display as volts with 2 dec. places
  lcd.write('V');
#else
  lcd.print(doseRatio,0);               // display conversion ratio in use
  lcd.print(F(" CPM=1 "));
  lcdprint_P((const char *)pgm_read_word(&(unit_lcd_table[doseUnit])));  // display units used for dose
  lcd.setCursor(0,1);                   // set cursor on line 2
  lcd.print(F("Running at "));          // display it
  lcd.print(readVcc() /1000. ,2);       // display as volts with 2 dec. places
  lcd.write('V');
#endif
  delay (2000);                         // leave info up for a bit
#if (DEBUG)                             // show available SRAM if DEBUG true
  clearDisp();
  lcd.print(F("RAM Avail: "));
  lcd.print(AvailRam());
  delay (2000);
#endif


  // this section tests if button pressed to set alarm threshold and calls a function to change it
  clearDisp();                          // put up new alarm set screen
#if (!EIGHT_CHAR)
  lcd.print(F("Set "));
#endif
  lcd.print(F("Alarm?"));
  lcd.setCursor(0, 1);
  lcd.print(F("Now ")); 
  if (AlarmPoint >0){
    lcd.print(AlarmPoint); 
#if (!EIGHT_CHAR)
    if (alarmInCPM) {
      lcd.print(F(" CPM"));
    } 
    else {
      lcd.write(' ');
      lcdprint_P((const char *)pgm_read_word(&(unit_lcd_table[doseUnit])));  // show dose unit (uSv/h, uR/h, or mR/h)
    }
#endif
  } 
  else lcd.print(F("Off")); 

  unsigned long timeIn = millis();      // you have 3 sec to push button or move on
  while (millis() < timeIn + 3000) { 
    if (readButton(SEL_BUTTON)== LOW) {
      setAlarm();                       // alarm is to be set
    }
  }

  // if no button press continue
  clearDisp();                          // clear the screen
#if (EIGHT_CHAR)
  lcd.print(F("??? "));
  lcd.write(4);                         // "CPM" custom character
#else
  lcd.print(F("CPM? "));                // display beginning "CPM"
#endif
  if (!radLogger){                      // no header if Radiation Logger is used
    Serial.print(F("CPM,"));            // print header for CSV output to serial
    serialprint_P((const char *)pgm_read_word(&(unit_table[doseUnit])));  // print dose unit (uSv/h, uR/h, or mR/h) to serial
    Serial.print(F(",Vcc\r\n"));
  }

  if (doseUnit == 0) {
    globalBgRadAvg = AVGBGRAD_uSv;      // global average background radiation in uSv/h
  } 
  else if (doseUnit == 1) {
    globalBgRadAvg = AVBGRAD_uR;        // global average background radiation in uR/h
  } 
  else {
    globalBgRadAvg = AVBGRAD_mR;        // global average background radiation in mR/h
  }
  nullPoint = 2 * doseRatio * globalBgRadAvg;  // set initial tone zero point to twice the global avg background radiation CPM
  dispPeriodStart = millis();           // start timing display CPM
  logPeriodStart = dispPeriodStart;     // start logging timer
  oneMinCountStart = dispPeriodStart;   // start 1 min scaler timer
  fastCountStart = dispPeriodStart;     // start bargraph timer
  longPeriodStart = dispPeriodStart;    // start long period scaler timer
  dispCnt = 0;                          // start with fresh totals
  logCnt= 0;
  oneMinCnt = 0;
  longPeriodCnt = 0;
  fastCnt = 0;
}

void loop(){
  static boolean scalerDispOn = false;  // true when SW_1 on, and in continous count mode
  static unsigned long lastButtonTime;  // counter for pressing the button to quickly
  static boolean scalerDispUsed = false;
#if (TONE_POT_ADJ)
  static unsigned int lastPotVal = 0;
  unsigned int potVal;
#endif



#if (SELF_TEST) // This will not give the "expected" 360 CPM. Simulated events are not an interrupt.
  dispCnt++;    // Since processing delays are included, it will settle at  about 348 CPM.
  logCnt++;
  oneMinCnt++;
  longPeriodCnt++;
  fastCnt++;
  delay(167);                           // 167 mS = 6 Hz `= X 60 = 360 CPM  
#endif

  Check_IR();                           // check if IR received a command

  if (readButton(SEL_BUTTON)== LOW && millis() >= lastButtonTime + 500){ // wait a bit between button pushes
    lastButtonTime = millis();          // reset the period time
    if(AlarmOn == false || alarmSilence == true) toggleScaler();
    if (AlarmOn == true) {              // maybe it was pressed to silence the alarm
      digitalWrite(ALARM_PIN, LOW);     // turn off alarm 
      alarmSilenceStart = millis();     // start the timer for the silence period
      alarmSilence = true;              // set a flag
    }
  }

  if (scalerParam) {
    if (!scalerDispOn) {
      clearDisp();                      // clear the screen when switching displays
      scalerDispOn = true;              // toggle scalerDispOn state
      if (scalerDispUsed == false){
        scalerDispUsed = true;          // false until first used
        oneMinCnt = longPeriodCnt = 0;  // clear counts
        longPeriodStart = oneMinCountStart = millis();
      }
      DispRunCounts();
    }
  } 
  else if (scalerDispOn) {
    clearDisp();
    scalerDispOn = false;
    DispCounts(dispCnt);                // start main display immediately
  }


  if (setNullPoint || (readButton(NULL_BUTTON)== LOW && millis() >= lastButtonTime + 500)){  // start/stop alt display mode if button pin is low
    lastButtonTime = millis();          // reset the period time
    setNullPoint = false;               // reset the flag
    nullPoint = 1.2 * currentDispCPM;   // set the nullPoint to 120% of the displayed CPM
  }

  if (millis() >= fastCountStart + 1000/ONE_SEC_MAX){ // refresh bargraph and alarm if in main display
    oneSecCount(fastCnt);
    fastCnt=0;                          // reset counts
    fastCountStart = millis();          // reset the period time
    if (!scalerDispOn) fastDisplay(getOneSecCount());      // display quick response data       

#if (TONE_POT_ADJ)
    potVal = analogRead(TONE_POT - 14);
    if (potVal > lastPotVal + POT_HYSTERESIS || potVal < lastPotVal - POT_HYSTERESIS) {        // don't keep re-adjusting the toneSensitivity due to normal fluctuations in reading
      toneSensitivity = round(((float)potVal*(float)potVal)/(1046529.0/(float)TONE_MAX_SENS)); // square the value and divide by 1023^2 scaled to TONE_MAX_SENS
      lastPotVal=potVal;
    }
#if (DEBUG)
    lcd.setCursor(12,1);
    lcd.print(toneSensitivity);
    lcd.print("   ");
#endif
#endif

    CPStoTone(getOneSecCount());
  }

  if (scalerDispUsed && millis() >= oneMinCountStart + 60000/ONE_MIN_MAX){ // Collect running counts every x sec.
    oneMinCount(oneMinCnt);             // add counts
    oneMinCnt = 0;                      // reset counts
    oneMinCountStart = millis();        // reset the period time
  }

  if (scalerDispUsed && millis() >= longPeriodStart + (scalerPeriod*60000)/LONG_PER_MAX && scalerPeriod < INFINITY) {
    longPeriodCount(longPeriodCnt);
    longPeriodCnt = 0;
    longPeriodStart = millis();
  }

  if (millis() >= dispPeriodStart + dispPeriod){ // DISPLAY PERIOD
    doseRatio = readCPMtoDoseRatio();        // check to see if TUBE_SEL has been changed
    if (readVcc() <= LOW_VCC) lowVcc = true; // check if Vcc is low 
    else lowVcc = false;

    // the scaler count screen will display if we're in scaler mode
    if (scalerDispOn) {
      DispRunCounts();                  // display the scaler mode screen
    } 
    else {
      DispCounts(dispCnt);              // period is over - display counts
    }
    dispCnt = 0;                        // reset counter
    dispPeriodStart = millis();         // reset the period time  
  }

  if (millis() >= logPeriodStart + LoggingPeriod && LoggingPeriod > 0){ // LOGGING PERIOD
    logCount(logCnt);                   // pass in the counts to be logged
    logCnt = 0;                         // reset log event counter
    logPeriodStart = millis(); // reset log time and display time too
  }
}


void DispCounts(unsigned long dcnt){    // calc and display predicted CPM & uSv/h
  float uSv = 0.0;                      // display CPM converted to VERY APPROXIMATE uSv
  unsigned long dispCPM;                // display CPM
  static float avgCnt;                  // holds the previous moving average count
  static byte sampleCnt;                // the number of samples making up the moving average
  byte maxSamples = (60000 / dispPeriod) / 2;   // number of sample periods in 30 seconds                     

  sampleCnt++;                                  // inc sample count - must be at least 1
  avgCnt += (dcnt - avgCnt) / sampleCnt;        // CALCULATE AVERAGE COUNT - moving average
  dispCPM = (avgCnt * 60000.0) / dispPeriod;    // convert to CPM

  //handle reset of sample count - sample is for 1/2 min and reset. Options for reset value are:
  // "0" - throw away last average, "1" - keeps last average, "maxSamples -1" - keeps running avg.
  if (sampleCnt >= maxSamples) sampleCnt = 0;   // start a fresh average every 30 sec.

  // The following line gives a faster response when counts increase or decrease rapidly 
  // It resets the running average if the rate changes by  +/- 35% (previously it was 9 counts)
  if ((dcnt - avgCnt) > (avgCnt * .35) || (avgCnt - dcnt) > (avgCnt * .35)) sampleCnt = 0;
  uSv = float(dispCPM) / doseRatio;     // make dose rate conversion
  //Blink(LED_PIN,1);                   // uncomment to blink each didplay

#if (ANDROID)
  Android.receive();                    // looks for new input from Android
  // don't send data via BT if not using app (no input) else it screws up serial output
  if (androidReturn >0)Android.send((dispCPM * androidReturn) / 100);
#endif

#if (EIGHT_CHAR)    // FOR 2x8 LCD FORMAT
  // display the "normal" count screen on the LCD
  clearDisp();                          // clear the screen
  lcd.print(dispCPM);                   // display CPM on line 1
  lcd.write(' ');
  lcd.write(4);                         // "CPM" custom character

  clearArea (0,1,8);                    // clear line 2
  printDoseRate(uSv,1,0);               // print dose rate on second line
  lcd.write(' ');
  lcd.write(1);                         // "uSv" custom character
#else
  clearArea (0,0,10);                   // clear count area
  lcd.print(F("CPM "));                 // display static "CPM"
  clearArea (0,1,16);                   // clear line 2
  lcdprint_P((const char *)pgm_read_word(&(unit_lcd_table[doseUnit])));  // print dose unit
  lcd.write(' ');
  printDoseRate(uSv,1,0);               // print dose rate on second line
  lcd.setCursor(4,0);                   // CPM LAST TO DISPLAY - NEVER PARTIALLY OVERWRITTEN
  lcd.print(dispCPM);                   // display CPM on line 1
#endif

  currentDispCPM = dispCPM;             // save the current CPM display in case the user sets the null point
  if (millis() > alarmSilenceStart + SILENCE_ALARM_PERIOD)alarmSilence = false ; 
  if (AlarmPoint > 0) {
    if (alarmInCPM) {                   // Alarm set to CPM
      if (dispCPM > AlarmPoint)  {
        AlarmOn = true;                 // for ALARM display
        // set alarm pin to HIGH if out of silence period 
        if (!alarmSilence) digitalWrite(ALARM_PIN, HIGH); 
      }
      if (dispCPM < AlarmPoint) {
        digitalWrite(ALARM_PIN, LOW);   // turn off alarm (set alarm pin to Gnd)
        AlarmOn = false;  
        alarmSilence = false ;          // reset for next alarm
      }
    } 
    else {                              // Alarm Set to Units
      if (uSv > AlarmPoint)  {          
        AlarmOn = true;                 // for ALARM display
        // set alarm pin to HIGH if out of silence period 
        if (!alarmSilence) digitalWrite(ALARM_PIN, HIGH);      
      }
      if (uSv < AlarmPoint) {
        digitalWrite(ALARM_PIN, LOW);   // turn off alarm (set alarm pin to Gnd)
        AlarmOn = false;  
        alarmSilence = false ;          // reset for next alarm
      }
    }
  }
}


void fastDisplay(unsigned long barCnt){ // quick response display on 2nd half of line 1
  barCnt = barCnt * 60;                 // scale CPS to CPM 
#if (!EIGHT_CHAR)    // NOT IN 2x8 LCD FORMAT
  if (!AlarmOn){
    clearArea (10,0,6);                 // move cursor to 9th col, 1st line for lcd bar
    printBar(barCnt, bargraphMax, 6);
  }
#endif
  if (lowVcc) {                         // overwrite display with battery voltage if low
    clearArea (11,0,5);
    lcd.print(lastVCC/1000.,2);         // display as volts with 2 dec. place
    lcd.write('V');
  }
  if (AlarmOn) {                        // overwrite display with alarm if on
    clearArea (10,0,6);
    lcd.setCursor(11,0);
    lcd.print(F("ALARM"));
  } 
#if (ANALOG_METER)
  DoseToMeter(barCnt);                  // Send CPM from bargraph to analog meter functions
#endif
}


void DispRunCounts(){ // create the screen that shows the running counts
  float tempSum;                        // for summing running count
  float temp_uSv;                       // for converting CPM to uSv/h for running average
  unsigned int secLeft;

  clearDisp();

  // 1 MINUTE DISPLAY LINE . . .
  tempSum = getOneMinCount();
  temp_uSv = tempSum / doseRatio;       // calc uSv/h
  if (!dispOneMin) {
    tempSum += oneMinCnt;
  }

  currentDispCPM = tempSum;             // save the currently displayed CPM in case the user sets the null point

#if (EIGHT_CHAR)    // FOR 2x8 LCD FORMAT
  lcd.write(2);                         // display 1 min icon
  lcd.write(' ');
  lcd.print(tempSum,0);                 // display 1 minute CPM or running count
#else
  lcd.print(F(" 1M       /"));          // display 1 & 10 min lits
  lcd.setCursor(0,1);
  if (scalerPeriod == INFINITY) {
    lcd.print("  \xf3");                // if scalerPeriod is set to INFINITY, write the symbol for infinity to the lcd
  } 
  else {
    if (scalerPeriod < 10) lcd.write(' ');
    lcd.print(scalerPeriod,DEC);
    lcd.print(F("M       /"));
  }

  lcd.setCursor(4, 0);
  lcd.print(tempSum,0);                 // display 1 minute CPM or running count
  if (dispOneMin) {
    printDoseRate(temp_uSv,0,1);        // display 1 minute uSv, right justified
  }
#endif
  if (!dispOneMin) {
#if (EIGHT_CHAR)    // FOR 2x8 LCD FORMAT
    lcd.setCursor(0, 0);
    lcd.write('C');                     // overwrite 10 minute icon with C if counting
#else
    secLeft = 60 - (oneMinuteIndex*60/ONE_MIN_MAX);
    if (secLeft < 10) {
      lcd.setCursor(14, 0);
    } 
    else {
      lcd.setCursor(13, 0);
    }
    lcd.print(secLeft,DEC);             // show seconds left
    lcd.write('s'); 
#endif
  }
  // 10 MINUTE DISPLAY LINE . . .
  tempSum = getLongPeriodCount();
  if (dispLongPeriod) {
    tempSum /= (float)scalerPeriod;   // sum over 10 minutes so divide by that when CPM is displayed
  } 
  else {
    tempSum += longPeriodCnt; // period hasn't finished yet add the current counts to the total
  }
  temp_uSv = tempSum / doseRatio;
#if (EIGHT_CHAR)    // FOR 2x8 LCD FORMAT
  lcd.setCursor(0, 1);
  lcd.write(3);                         // display 10 min icon
  lcd.write(' ');
  lcd.print(tempSum,0);                 // display 1 minute CPM or running count
#else
  lcd.setCursor(4, 1);
  lcd.print(tempSum,0);                 // display long period CPM
  if (dispLongPeriod) {
    printDoseRate(temp_uSv,1,1);        // display long period dose rate, right justified
  }
#endif
  if (!dispLongPeriod && scalerPeriod < INFINITY) {
#if (EIGHT_CHAR)    // FOR 2x8 LCD FORMAT
    lcd.setCursor(0, 1);
    lcd.write('C');                     // overwrite 10 minute icon with C if counting
#else
    // Type casting needed to prevent unsigned int overflow at longPeriodIndex==110 when scaler period is 10 minutes (109*10*60=65400)
    secLeft = (scalerPeriod * 60) - (((unsigned long)longPeriodIndex*(unsigned long)scalerPeriod*(unsigned long)60)/(unsigned long)LONG_PER_MAX);



    if (secLeft > 600) {  // longer than 10 min left, show minutes
      lcd.setCursor(13, 1);
      lcd.print(secLeft/60,DEC);        // show minutes left
      lcd.write('m'); 
    } 
    else {
      if (secLeft < 10) {
        lcd.setCursor(14, 1);
      } 
      else if (secLeft < 100) {
        lcd.setCursor(13, 1);
      } 
      else {
        lcd.setCursor(12, 1);
      }
      lcd.print(secLeft,DEC);
      lcd.write('s');
    }
#endif
  }
}


unsigned long getOneSecCount() {
  unsigned long tempSum = 0;
  for (int i = 0; i <= ONE_SEC_MAX-1; i++){ // sum up 1 second counts
    tempSum = tempSum + oneSecond[i];
  }
  return tempSum;
}


unsigned long getOneMinCount() {
  unsigned long tempSum = 0;
  for (int i = 0; i <= ONE_MIN_MAX-1; i++){ // sum up 1 minute counts
    tempSum = tempSum + oneMinute[i];
  }
  return tempSum;
}


unsigned long getLongPeriodCount() {
  unsigned long tempSum = 0;
  for (int i = 0; i <= LONG_PER_MAX-1; i++){ // sum up long period counts
    tempSum = tempSum + longPeriod[i];
  }
  return tempSum;
}


void setAlarm(){ // RECURSIVE FUNCTION to change alarm set point when button repeatidly pushed
  unsigned long timeIn = millis();      // capture the time you got here

  while (millis() < timeIn + 2000) {    // you got 2 sec. to push button again - else done
    if (readButton(SEL_BUTTON)== LOW){  // button pushed
      if (AlarmPoint < 10) AlarmPoint += 1;              // inc by 1 up to 10
      else if (AlarmPoint < 100) AlarmPoint += 10;       // inc by 10 up to 100
      else if (AlarmPoint < 1000) AlarmPoint += 100;     // inc by 100 over 100
      else AlarmPoint += 1000;                           // inc by 1000 over 1000
      if (AlarmPoint > MAX_ALARM) AlarmPoint = 0;        // start over if max point reached - zero is off
      lcd.setCursor(0, 1);                               // display what's going on
      clearArea (0,1,16);
      if (AlarmPoint > 0){
        lcd.print(AlarmPoint);
        lcd.write(' ');
        if (alarmInCPM) {  // determine whether alarm is being set in CPM or in dose units
          lcd.print(F("CPM"));
        } 
        else {
          lcdprint_P((const char *)pgm_read_word(&(unit_lcd_table[doseUnit])));  // show unit (uSv/h, uR/h, mR/h)
        }
        delay(500);
      } 
      else {
        lcd.print(F("Alarm Off")); 
        delay(3000);
      }
      setAlarm();                       // call this function recursively if button was pushed
    }
  }                                                      // button not pushed - done use last setting for alarm point
  writeParam(AlarmPoint, ALARM_SET_ADDR);                // store new setting in EEPROM
}


static void printBar(unsigned long value, unsigned long max, byte blocks) {
  // NEW STYLE - Adapted from zxcounter by Andrei K. - https://github.com/andkom/zxcounter
  // modified to remove floating point math
  byte bar_value, full_blocks, prtl_blocks;

  if (value > max) {
    full_blocks = blocks;
  } 
  else {
    bar_value = (blocks * 5 * value) / max;
    full_blocks = bar_value / 5;
    prtl_blocks = bar_value % 5;
  }

  for (byte i = 0; i < full_blocks; i++) {
    lcd.write(5);
  }

  lcd.write(prtl_blocks);

  for (byte i = full_blocks + 1; i < blocks; i++) {
    lcd.write(byte(0));
  }
}


void logCount(unsigned long lcnt){ // unlike logging sketch, just outputs to serial
  unsigned long logCPM;                 // log CPM
  float uSvLogged = 0.0;                // logging CPM converted to "unofficial" uSv

  logCPM = float(lcnt) / (float(LoggingPeriod) / 60000);
  uSvLogged = (float)logCPM / doseRatio; // make uSV conversion

  // Print to serial in a format that might be used by Excel
  Serial.print(logCPM,DEC);
  if (!radLogger){                      // only CPM if Radiation Logger is used
    Serial.write(',');    
    Serial.print(uSvLogged,4);
    Serial.write(','); // comma delimited
    Serial.print(readVcc()/1000. ,2);   // print as volts with 2 dec. places
  }
  Serial.print(F("\r\n"));
  Blink(LED_PIN,2);                     // show it logged
}


void oneSecCount(unsigned long dcnt) {
  static byte oneSecondIndex = 0;

  oneSecond[oneSecondIndex++] = dcnt;
  if(oneSecondIndex >= ONE_SEC_MAX) {
    oneSecondIndex = 0;
  }
}


void resetOneMinCount() {  // clears out the one minute count
  memset(oneMinute, 0, sizeof(oneMinute));  // zero the entire array
  oneMinuteIndex=0;                     // reset index to 0
  dispOneMin = false;                   // clear the flag
  oneMinCnt = 0;                        // reset the running count
  oneMinCountStart = millis();          // reset the running count start time
}


static void oneMinCount(unsigned long dcnt){ // Add CPM of period to 1M 
  oneMinute[oneMinuteIndex] = dcnt;
  if(oneMinuteIndex >= ONE_MIN_MAX-1) {
    oneMinuteIndex = 0;
    if (!dispOneMin) {
      dispOneMin = true;                // indicate that average is available
    }
  }
  else oneMinuteIndex++;
}


void resetLongPeriodCount() {  // resets the one long period count
  memset(longPeriod, 0, sizeof(longPeriod));  // zero the entire array
  longPeriodIndex=0;                    // reset index to 0
  dispLongPeriod = false;               // clear the flag
  longPeriodCnt = 0;                    // reset the running count
  longPeriodStart = millis();           // reset the running count start time
}


static void longPeriodCount(unsigned long dcnt){ // Add CPM of period to 1M 
  longPeriod[longPeriodIndex] = dcnt;
  if(longPeriodIndex >= LONG_PER_MAX-1) {
    longPeriodIndex = 0;
    dispLongPeriod = true;              // indicate that average is available
  }
  else longPeriodIndex++;
}


unsigned long readVcc() { // SecretVoltmeter from TinkerIt
  unsigned long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  lastVCC = result;           // TO DO - NOW MAKING ENOUGH CALLS TO WARRENT THIS - USING W/ METER
  return result;
}


static void CPStoTone (unsigned long counts){
  unsigned long scaleMax;
  // 40 ohm speaker draws ~33mA
  if (counts*60 <= nullPoint || !PiezoOn) {
    noTone(TONE_PIN); // no counts, no tone 
    return;
  } 
  else {
    counts -= nullPoint/60;             // remove nullpoint from count
  }
  if (toneSensitivity > 0) {
    // no real math behind this formula - obtained by experimentation; seems to produce better results than a straight linear function
    counts = 1 + ((counts+counts*counts)/(unsigned long)toneSensitivity);      
  } 
  else {
    counts = 1 + (counts + counts*counts*counts-counts*counts)/3;   // same as above, but different formula for even more sensitivity
  }
  scaleMax = bargraphMax;            // use the full scale setting for the bargraph as the max for the tone range
  tone(TONE_PIN, lmap(counts,1,scaleMax,TONE_MIN_FREQ,TONE_MAX_FREQ));
}


// rolling your own map function saves a lot of memory
unsigned long lmap(unsigned long x, unsigned long in_min, unsigned long in_max, unsigned long out_min, unsigned long out_max){
  return x>in_max ? out_max : (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//----------------------------------------------------------------------------------------------+
//                                      Utilities
//----------------------------------------------------------------------------------------------+

static void clearArea (byte col, byte line, byte nspaces){
  // starting at col & line, prints n spaces then resets the cursor to the start
  lcd.setCursor(col,line);
  for (byte i=0; i<nspaces; i++){
    lcd.write(' ');
  }
  lcd.setCursor(col,line);
}


void printDoseRate (float rate, byte line, boolean rightJustify) {  // prints the uSv/hour rate on the right side of the LCD
  /* Prints 2 decimal places if uSv is less than 10, 1 decimal place if 
   less than 100, and only whole numbers if the uSv rate is 100 or more.  This allows the LCD to 
   clearly display dose rates up to 99,999uSv/hr without causing any display weirdness.*/
  byte startpos;
  byte precision;

  if (rate < 10) {                      // display 2 decimal places if less than 10
    startpos=13;
    precision=2;
  } 
  else if (rate < 100) {                // display 1 decimal place if less than 100
    startpos=14;
    precision=1;
  } 
  else {                                // display only whole numbers if 100 or more
    precision=0;
    startpos=16;
  }
  if (rightJustify) lcd.setCursor(startpos - getLength(rate),line); // right justify the dose rate!
  lcd.print(rate,precision);            // display dose rate
}


void clearDisp (){
  // The OLED display does not always reset the cursor after a clear(), so it's done here
  lcd.clear();                          // clear the screen
  lcd.setCursor(0,0);                   // reset the cursor for the poor OLED
  lcd.setCursor(0,0);                   // do it again for the OLED
}


void Blink(byte led, byte times){ // just to flash the LED
  for (byte i=0; i< times; i++){
    digitalWrite(led,HIGH);
    delay (150);
    digitalWrite(led,LOW);
    delay (100);
  }
}


// variables created by the build process when compiling the sketch
extern int __bss_end;
extern void *__brkval;

int AvailRam(){ 
  int freeValue;
  if ((int)__brkval == 0)
    freeValue = ((int)&freeValue) - ((int)&__bss_end);
  else
    freeValue = ((int)&freeValue) - ((int)__brkval);
  return freeValue;
} 


byte getLength(unsigned long number){
  byte length = 0;
  unsigned long t = 1;
  do {
    length++;
    t*=10;
  } 
  while(t <= number);
  return length;
}


byte readButton(int buttonPin) { // reads LOW ACTIVE push buttom and debounces
  if (digitalRead(buttonPin)) return HIGH;    // still high, nothing happened, get out
  else {                                      // it's LOW - switch pushed
    delay(DEBOUNCE_MS);                       // wait for debounce period
    if (digitalRead(buttonPin)) return HIGH;  // no longer pressed
    else return LOW;                          // 'twas pressed
  }
}


#if (ANDROID)
void androidInput(byte flag, byte numOfValues){ // automatically called when input from Android
  androidReturn = Android.getInt();           // set global with value from slider
}
#endif


void lcdprint_P(const char *text) {  // print a string from progmem to the LCD
  /* Usage: lcdprint_P(pstring) or lcdprint_P(pstring_table[5].  If the string 
   table is stored in progmem and the index is a variable, the syntax is
   lcdprint_P((const char *)pgm_read_word(&(pstring_table[index])))*/
  while (pgm_read_byte(text) != 0x00)
    lcd.write(pgm_read_byte(text++));
}


static void serialprint_P(const char *text) {  // print a string from progmem to the serial object
  /* Usage: serialprint_P(pstring) or serialprint_P(pstring_table[5].  If the string 
   table is stored in progmem and the index is a variable, the syntax is
   serialprint_P((const char *)pgm_read_word(&(pstring_table[index])))*/
  while (pgm_read_byte(text) != 0x00)
    Serial.write(pgm_read_byte(text++));
}


//----------------------------------------------------------------------------------------------+
//                                        ISR
//----------------------------------------------------------------------------------------------+

void GetEvent(){   // ISR triggered for each new event (count)
  dispCnt++;
  logCnt++;
  oneMinCnt++;
  longPeriodCnt++;
  fastCnt++;
}






