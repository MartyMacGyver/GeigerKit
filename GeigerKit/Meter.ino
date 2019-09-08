//----------------------------------------------------------------------------------------------+
//                               Functions for Analog Meter if defined
//----------------------------------------------------------------------------------------------+


#if (ANALOG_METER)  // THIS TAB NOT USED UNLESS ANALOG METER IS DEFINED
void DoseToMeter(unsigned long value){
  // 50uA meter through 100K load = 5V full scale = 255 to D/A.
  // Vcc may be <5V so compensate in lmap (meter still must be zeroed)
  unsigned int swPos;
  unsigned long scale = 1;
  unsigned long meter;

  swPos = Get_SwPos();         // get the position of the "Range" rotary switch

  if (swPos ==-1) {             // switch is off
    PCF8591_DA(0);
    return;
  }

  if (swPos ==0) {             // send the DAC a 255 for full scale if switch in cal. position
    PCF8591_DA(255);
    return;
  }

  for (int i = 1; i<swPos; i++){ // convert switch position to a multiple of scale
    scale = scale * 10;
  }
  scale = 1000/scale;            // convert to meter units for X0.1, X1, X10, X100
  meter = (value * scale) / (doseRatio);         // make dose rate conversion
  PCF8591_DA(lmap(meter,0,(lastVCC/10),0,255));  // map to DAC value and send it to PCF8591
}


unsigned int Get_SwPos(){ // Convert ADC value from rotary switch to zero based position number
  int k;
  PCF8591_AD();                // read A/D to get settings
  for (k = 0; k < NUM_KEYS; k++) {
    if (digIn[1] < adc_key_val[k]) return k;
  }
  return -1;                   // switch is off
}


/////////////////////  DAC and ADC functions for PCF8591 chip ///////////////////////
void PCF8591_DA(byte digOut){  // Digital into chip, Analog out 
  Wire.beginTransmission(ADDA_ADDR);  // Connect to PCF8591
  Wire.write(0x40); 	              // Enable D/A output
  Wire.write(digOut);                 // send the digital data to convert to analog
  Wire.endTransmission(); 
}


void PCF8591_AD(){ // Analog into chip, Digital out 
  Wire.beginTransmission(ADDA_ADDR);  // Connect to PCF8591
  Wire.write(0x44);                    // D/A on (for int osc), autoincrement, 4 seperate 
  Wire.endTransmission(); 
  Wire.beginTransmission(ADDA_ADDR);  // connect to PCF8591 to read result
  Wire.requestFrom(ADDA_ADDR, 5);     // request 5 bytes from PCF8591
  if(5 <= Wire.available()){          // if 5 bytes were received . . .
    digIn[0]=Wire.read();             // throw 1st away - it was last read from previous call
    digIn[1]=Wire.read();             // read in the 4 analog inputs
    digIn[2]=Wire.read();
    digIn[3]=Wire.read();
    digIn[4]=Wire.read();
  }
  Wire.endTransmission(); 
}
#endif
