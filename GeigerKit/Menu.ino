///////////////////////////////// Menu and IR Functions Here ///////////////////////////////

void Check_IR(){ // check if remote used and process the menu    
  // IR_Dev not used - only IR_Cmnd - accept any device (i.e. TV, VCR, etc.) command.
  boolean inMenu = false;               // true when in menu
  byte IRdigit;                         // normalized digit input from remote
  byte MenuPos = 0;                     // current menu position 0 based
  float IRvalue = 0;                    // value built from digits sent from IR receiver
  boolean menuChanged = false;          // flag indicating whether the menu needs to be redrawn
  boolean directEntry = false;          // flag indicating whether the user is directly entering a numeric value
  unsigned int decimalDiv = 0;          // decimal divisor for entry of floating point numbers
  if(!IR_Avail)return;                  // just get out if a key on IR has not been pressed
  //detachInterrupt(0);                 // uncomment to not count while in menu
  do {
    if(!IR_Avail) continue;
    switch (IR_Cmnd){                     // a case for each func key if desired
      case 0 ... 9:                       // commands 0-9 converted to digits 0-9
        if (!inMenu) break;
        IRdigit = IR_Cmnd;
        if (!directEntry) {                   // detect if this is the first digit being entered
          directEntry=true;                   // set flag to tell us we're in direct entry mode
          IRvalue = IRdigit;                  // if the default value is present, overwrite it
        } else if (decimalDiv == 0) {
          IRvalue = IRvalue * 10 + IRdigit;   // build the value
        } else if (decimalDiv < 100) {
          decimalDiv *= 10;
          IRvalue = IRvalue + (float)IRdigit/(float)decimalDiv;   // build the value
        }
        break;
   
      case POWER:                           // begin / end menu mode
        inMenu = !inMenu;                   // toggle
        if (inMenu) {
#if (TONE_MODE)
          noTone(TONE_PIN);                 // silence is golden while in the menu
#endif
          menuChanged=true;
        } else {                            // do this stuff when leaving menu
          Get_Settings();                   // start using the new settings
        }
        break;
      
      case DOWN:                            // NEXT MENU
      case C_DOWN:
        if (!inMenu) break;
        MenuPos++;
        if (MenuPos > MAX_MENU) MenuPos = 0;
        menuChanged=true;
        break;

      case UP:                              // PREVIOUS MENU
      case C_UP:
        if (!inMenu) break;
        MenuPos--;
        if (MenuPos > MAX_MENU) MenuPos = MAX_MENU;
        menuChanged=true;
        break;

      case AVTV:                            // USED FOR DECIMAL POINT
      case DCML:
        if (MenuPos == MENU_RATIO) decimalDiv = 1;
        else decimalDiv = 0;
        break;
        
      case ENTER:                           // TERMINATE THE ENTRY
      case ENTER2:
      case EXT1:
      case PLAY:
      case KC_MENU:
        if (!inMenu) {
#if (TONE_MODE)
          setNullPoint = true;
#endif
          break;
        }
        saveMenuSetting(MenuPos, IRvalue);
        decimalDiv = directEntry = false;
        break;
    
      case RIGHT:                           // INCREMENT THE ENTRY
      case V_UP:
      case KC_UP:
        if (!inMenu) break;
        IRvalue = incrementMenuSetting(MenuPos, IRvalue);
        directEntry=false;
        break;
    
      case LEFT:                            // DECREMENT THE ENTRY
      case V_DOWN:
      case KC_DOWN:
        if (!inMenu) break;
        IRvalue = decrementMenuSetting(MenuPos, IRvalue);
        directEntry=false;
        break;
    
      case MUTE:                            // Mute the sound
        togglePiezo(!PiezoOn);
        break;
    
      case INFO:                            // --/- key i\on keychain
      case RECALL:                          // same for RM-EZ4
        toggleScaler();
        break;
        
      default:                              // DISPLAY CODE FOR ANY UNDEFINED KEY
        if (!inMenu) break;
        clearArea (0,1,16);
        lcd.print(F("Code "));
        lcd.print(IR_Cmnd,DEC);
        delay (1500);
        //digitalWrite(LED_PIN, LOW);         // turn off LED
    } // end switch

    IR_Avail = false;                     // allow IR again
    //digitalWrite(LED_PIN, LOW);           // turn off LED
    if (inMenu){                          // display the input . . .
      IRvalue = displayMenuScreen(MenuPos, IRvalue, menuChanged);
      if (menuChanged) {
        decimalDiv = directEntry = menuChanged = false;
      }
    }
  }while(inMenu);
}

///////////////////////////////////////////////////////////////////////
//  Menu helper functions
///////////////////////////////////////////////////////////////////////

float displayMenuScreen(byte menu, float curValue, boolean unchanged) {
  clearDisp();
  lcdprint_P((const char *)pgm_read_word(&(menu_table[menu])));
  lcd.setCursor(0,1);                   // set cursor on line 2
  /// DEBUG   lcd.print(menu_items[menu]);
  switch(menu) {
    case MENU_RATIO:
      lcd.setCursor(5,0);
      lcdprint_P((const char *)pgm_read_word(&(unit_lcd_table[doseUnit]))); // print proper display unit
      lcd.setCursor(0,1);
      if (unchanged) curValue = readCPMtoDoseRatio();
      lcd.print(curValue);
      lcd.setCursor(7,1);
      lcd.print("Now ");
      lcd.print(readCPMtoDoseRatio());
      break;
    case MENU_DOSE_UNIT:
      if (unchanged) curValue = doseUnit;
      curValue = (unsigned int)curValue % (MAX_UNIT + 1);
      lcdprint_P((const char *)pgm_read_word(&(unit_lcd_table[(unsigned int)curValue])));
      lcd.setCursor(7,1);
      lcd.print(F("Now "));
      lcdprint_P((const char *)pgm_read_word(&(unit_lcd_table[doseUnit])));
      break;
    case MENU_ALARM_UNIT:
      if (unchanged) curValue = alarmInCPM;
      curValue = (unsigned int)curValue % 2;
      if (curValue) {
        lcd.print(F("CPM"));
      } else {
        lcdprint_P((const char *)pgm_read_word(&(unit_lcd_table[doseUnit])));
      }
      lcd.setCursor(7,1);
      lcd.print(F("Now "));
      if (alarmInCPM) {
        lcd.print(F("CPM"));
      } else {
        lcdprint_P((const char *)pgm_read_word(&(unit_lcd_table[doseUnit])));
      }
      break;
    case MENU_BATT:                        // show the battery voltage
      lcd.setCursor(6,1);
      lcd.print(readVcc()/1000. ,2);       // convert to Float, divide, and print 2 dec places
      lcd.write('V');
      break;
    default:
      if (unchanged) curValue = readParam(menu*2);     // if this screen was just entered, set the default value to the current value
      printValues((unsigned int)curValue,readParam(menu*2));           // print the values on line 2
  }
  return curValue;
}

static void saveMenuSetting (byte menu, float curValue) {
  switch(menu) {
    case MENU_DISP_PER:
      if (curValue > DISP_PERIOD_MAX) curValue = DISP_PERIOD_MAX;
      if (curValue < DISP_PERIOD_MIN) curValue = DISP_PERIOD_MIN;
      writeParam(curValue, DISP_PERIOD_ADDR);
      break;
    case MENU_MIN_LOG:
      if (curValue > LOGGING_PERIOD_MAX) curValue = LOGGING_PERIOD_MAX;
      writeParam(curValue, LOG_PERIOD_ADDR);
      break;
    case MENU_RATIO:
      if (curValue > DOSE_RATIO_MAX) curValue = DOSE_RATIO_MAX;
      writeCPMtoDoseRatio(curValue);
      break;
    case MENU_ALARM:
      if (curValue > MAX_ALARM) curValue = MAX_ALARM;
      writeParam(curValue, ALARM_SET_ADDR);
      break;
    case MENU_DOSE_UNIT:
      doseUnit = curValue;
      writeParam(curValue, DOSE_UNIT_ADDR);
      break;
    case MENU_ALARM_UNIT:
      alarmInCPM=!alarmInCPM;
      writeParam(alarmInCPM,ALARM_UNIT_ADDR);
      break;
    case MENU_SCALER_PER:
      if (curValue >= SCALER_PER_MAX && curValue != INFINITY) curValue = SCALER_PER_MAX;
      else if (curValue <= SCALER_PER_MIN) curValue = SCALER_PER_MIN;
      else if (curValue != INFINITY) {
        while (curValue < SCALER_PER_MAX && (60000 * (unsigned int)curValue) % LONG_PER_MAX != 0) {  // have to make sure that the interval is evenly divisible by the number of elements in the array (if LONG_PER_MAX is left at 120, this is always false and optimized out by the compiler)
          curValue++;
        }
      }
      writeParam(curValue, SCALER_PER_ADDR);
      resetLongPeriodCount();        // reset the long period count because we changed the period
      break;
    case MENU_BARGRAPH_MAX:
      if (curValue > BARGRAPH_SCALE_MAX) curValue = BARGRAPH_SCALE_MAX;
      else if (curValue < BARGRAPH_SCALE_MIN) curValue = BARGRAPH_SCALE_MIN;
      writeParam(curValue, BARGRAPH_MAX_ADDR);
      break;
#if (TONE_MODE)
    case MENU_TONE_SENS:
      if (curValue > TONE_MAX_SENS) curValue = TONE_MAX_SENS;
      writeParam(curValue, TONE_SENS_ADDR);
      break;
#endif
    default:
      writeParam(curValue, menu*2);
  }
}

static float incrementMenuSetting (byte menu, float curValue) {
  switch(menu) {
    case MENU_DISP_PER:
      if (curValue>=DISP_PERIOD_MAX) curValue = DISP_PERIOD_MIN;
      else curValue++;
      break;
    case MENU_MIN_LOG:
      if (curValue>=LOGGING_PERIOD_MAX) curValue = 0;
      else curValue++;
      break;
    case MENU_RATIO:
      if (curValue>=DOSE_RATIO_MAX) curValue = 0;
      else curValue++;
      break;
    case MENU_ALARM:
      if (curValue>=MAX_ALARM) curValue = 0;
      else curValue++;
      break;
    case MENU_DOSE_UNIT:
      if (curValue>=MAX_UNIT) curValue = 0;
      else curValue++;
      break;
    case MENU_SCALER_PER:
      if (curValue==SCALER_PER_MAX) curValue = INFINITY;
      else if (curValue==INFINITY) curValue = SCALER_PER_MIN;
      else {
        do {
          curValue++;
        } while (curValue < SCALER_PER_MAX && (60000 * (unsigned int)curValue) % LONG_PER_MAX != 0);  // have to make sure that the interval is evenly divisible by the number of elements in the array (if LONG_PER_MAX is left at 120, this is always false and optimized out by the compiler)
      }
      break;
    case MENU_BARGRAPH_MAX:
      if (curValue>=BARGRAPH_SCALE_MAX) curValue = BARGRAPH_SCALE_MIN;
      else curValue++;
      break;
#if (TONE_MODE)
    case MENU_TONE_SENS:
      if (curValue>=TONE_MAX_SENS) curValue = 0;
      else curValue++;
      break;
#endif
    default:
      curValue++;
  }
  return curValue;
}

static float decrementMenuSetting(byte menu, float curValue) {
  switch(menu) {
    case MENU_DISP_PER:
      if (curValue<=DISP_PERIOD_MIN) curValue = DISP_PERIOD_MAX;
      else curValue--;
      break;
    case MENU_MIN_LOG:
      if (curValue==0) curValue = LOGGING_PERIOD_MAX;
      else curValue--;
      break;
    case MENU_RATIO:
      if (curValue==0) curValue = DOSE_RATIO_MAX;
      else curValue--;
      break;
    case MENU_ALARM:
      if (curValue==0) curValue = MAX_ALARM;
      else curValue--;
      break;
    case MENU_DOSE_UNIT:
      if (curValue==0) curValue = MAX_UNIT;
      else curValue--;
      break;
    case MENU_SCALER_PER:
      if (curValue==SCALER_PER_MIN) curValue = INFINITY;
      else if (curValue==INFINITY) curValue = SCALER_PER_MAX;
      else {
        do {
          curValue--;
        } while ((60000 * (unsigned int)curValue) % LONG_PER_MAX != 0);  // have to make sure that the interval is evenly divisible by the number of elements in the array (if LONG_PER_MAX is left at 120, this is always false and optimized out by the compiler)
      }
      break;
    case MENU_BARGRAPH_MAX:
      if (curValue==BARGRAPH_SCALE_MIN) curValue = BARGRAPH_SCALE_MAX;
      else curValue--;
      break;
#if (TONE_MODE)
    case MENU_TONE_SENS:
      if (curValue==0) curValue = TONE_MAX_SENS;
      else curValue--;
      break;
#endif
    default:
      curValue--;
  }
  return curValue;
}

static void printValues(unsigned int curVal, unsigned int newVal) {
  if (curVal==INFINITY) {
    lcd.write('\xf3');
  } else {
    lcd.print(curVal,DEC);
  }
  lcd.setCursor(7,1);
  lcd.print("Now ");
  if (newVal==INFINITY) {
    lcd.write('\xf3');
  } else {
    lcd.print(newVal);
  }
}

///////////////////////////////////////////////////////////////////////
// Functions to read settings from / write settings to EEPROM
///////////////////////////////////////////////////////////////////////

void Get_Settings(){ // read setting out of EEPROM and set local variables
  // set defaults if EEPROM has not been used yet
  dispPeriod = readParam(DISP_PERIOD_ADDR);
  if (dispPeriod == 0 || dispPeriod > DISP_PERIOD_MAX || RESET_ALL){      // default if > 1 hr
    writeParam(DISP_PERIOD,DISP_PERIOD_ADDR);    // write EEPROM
    dispPeriod = DISP_PERIOD;
  }

  if (readParam(SCALER_ADDR) > 1 || RESET_ALL) {
    writeParam(false,SCALER_ADDR);
  }
  scalerParam = (boolean)readParam(SCALER_ADDR);

  doseRatio = readCPMtoDoseRatio();

  LoggingPeriod = readParam(LOG_PERIOD_ADDR);
  if (LoggingPeriod > LOGGING_PERIOD_MAX || RESET_ALL){       // if zero, no logging - defult if > 24 hr
    writeParam(LOGGING_PERIOD,LOG_PERIOD_ADDR);  // write EEPROM
    LoggingPeriod = LOGGING_PERIOD;
  }
  LoggingPeriod *= 60000;

  AlarmPoint = readParam(ALARM_SET_ADDR);        // if zero - no alarm
  if (AlarmPoint > MAX_ALARM || RESET_ALL){                   // defult if > ALARM_MAX CPM
    writeParam(ALARM_POINT,ALARM_SET_ADDR);      // write EEPROM
    AlarmPoint = ALARM_POINT;
  }

  doseUnit = readParam(DOSE_UNIT_ADDR);          // get the saved value for the dose unit
  if (doseUnit > MAX_UNIT || RESET_ALL) {
    writeParam(0, DOSE_UNIT_ADDR);               // default to uSv
    doseUnit = 0;
  }

#if (RESET_ALL)
  writeParam(1,ALARM_UNIT_ADDR);
#endif
  alarmInCPM = (boolean)readParam(ALARM_UNIT_ADDR);

#if (RESET_ALL)
  writeParam(1,PIEZO_SET_ADDR);
#endif
  PiezoOn = (boolean)readParam(PIEZO_SET_ADDR);  // set the piezo to the last status

#if (TONE_MODE)
  toneSensitivity = readParam(TONE_SENS_ADDR);
  if (toneSensitivity > TONE_MAX_SENS || RESET_ALL) {
    writeParam(TONE_SENSITIVITY, TONE_SENS_ADDR);
    toneSensitivity = TONE_SENSITIVITY;
  }
#endif

  scalerPeriod = readParam(SCALER_PER_ADDR);     // get the saved value for the long period scaler
  if (scalerPeriod < SCALER_PER_MIN || scalerPeriod > SCALER_PER_MAX || (60000 * scalerPeriod) % LONG_PER_MAX != 0 || RESET_ALL) { // discard the value if over the max or if not divisible by the number of elements in the array
    if (scalerPeriod != INFINITY) {
      writeParam(SCALER_PERIOD, SCALER_PER_ADDR);  // write default value
      scalerPeriod = SCALER_PERIOD;
    }
  }

  bargraphMax = readParam(BARGRAPH_MAX_ADDR);    // get the CPM value that will put the bargraph at full scale - if not previously set, use the default
  if (bargraphMax > BARGRAPH_SCALE_MAX || RESET_ALL) {
    writeParam(FULL_SCALE, BARGRAPH_MAX_ADDR);
    bargraphMax = FULL_SCALE;
  }
  
  dispPeriodStart = 0;                  // start timing over when returning to loop
  logPeriodStart = dispPeriodStart;     // start logging timer
}

void writeParam(unsigned int value, unsigned int addr){ // Write menu entries to EEPROM
  unsigned int a = value/256;
  unsigned int b = value % 256;
#if (GPS_USED) // store GPS specific params 48 bytes out
  if (addr >=12) addr = addr + 48;
#endif
  EEPROM.write(addr,a);
  EEPROM.write(addr+1,b);
}

unsigned int readParam(unsigned int addr){ // Read previous menu entries from EEPROM
#if (GPS_USED) // store GPS specific params 48 bytes out
  if (addr >=12) addr = addr + 48;
#endif
  unsigned int a=EEPROM.read(addr);
  unsigned int b=EEPROM.read(addr+1);
  return a*256+b; 
}

void writeFloatParam(float value, unsigned int addr) {
  const byte* p = (const byte*)(const void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++)
    EEPROM.write(addr++, *p++);
  return;
}

static float readFloatParam(unsigned int addr) {
  float value;
  byte* p = (byte*)(void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(addr++);
  return value;
}

static void toggleScaler() {
  scalerParam = !scalerParam;
  writeParam(scalerParam, SCALER_ADDR);   // save the setting in EEPROM
}

static void togglePiezo(boolean bState){      // toggle piezo control pin
  PiezoOn = bState;
  if (PiezoOn) {                        // if ON - set the pin to float 
    pinMode(PIEZO_SIG_PIN, INPUT);      // set the pin to input to make it float (high Z)
  } else {                              // it's OFF - set the pin to LOW
    pinMode(PIEZO_SIG_PIN, OUTPUT);
    digitalWrite(PIEZO_SIG_PIN,LOW);
  }
  writeParam(PiezoOn, PIEZO_SET_ADDR);  // save the setting in EEPROM
}

float readCPMtoDoseRatio() {
  unsigned int addr;
  float ratio;
  float defaultRatio;
  
  if (digitalRead(TUBE_SEL)) {                  // determine if the tube select switch is open or closed
    addr = PRI_RATIO_ADDR;                      // switch is open.  Use the primary tube ratio
    defaultRatio = PRI_RATIO;
  } else {                                      // switch is closed.  Use the secondary tube ratio
    addr = SEC_RATIO_ADDR;
    defaultRatio = SEC_RATIO;
  }
#if (RESET_ALL)
  writeFloatParam(PRI_RATIO,PRI_RATIO_ADDR);
  writeFloatParam(SEC_RATIO,SEC_RATIO_ADDR);
#endif
  ratio = readFloatParam(addr);
  if (ratio == 0 || ratio > DOSE_RATIO_MAX || isnan(ratio)) {  // defult if 0 or > 2000
    writeFloatParam(defaultRatio,addr);         // write EEPROM
    ratio = defaultRatio;                       // set the default
  }
  return ratio;
}

void writeCPMtoDoseRatio(float ratio) {
  if (digitalRead(TUBE_SEL)) {                  // determine if the tube select switch is open or closed
    writeFloatParam(ratio, PRI_RATIO_ADDR);     // write to the primary address
  } else {
    writeFloatParam(ratio, SEC_RATIO_ADDR);     // write to the secondary address
  }
  resetOneMinCount();                           // reset the counts since we're changing the ratio - otherwise, they'll be off
  resetLongPeriodCount();
}
