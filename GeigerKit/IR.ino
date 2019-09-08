#if (!IR_RC5) // Sony Remote is used
void IR_ISR(){ // This ISR is called for EACH pulse of the IR sensor
  static boolean IR_Header = false;     // flag set if IR header was received
  static unsigned int ir_mask = 0;      // for masking ir_string 
  static unsigned int ir_bit_seq = 0;   // for bit number in ir_string
  static unsigned int ir_string = 0;    // stores bits received
  if(IR_Header == false){               // check for the long start pulse
    for(int i = 0; i<20; i++){          // see if it lasts at least 2 ms
      delayMicroseconds(100);           // delay in chunks to get out ASAP
      if(digitalRead(IR_PIN)) return;   // low active went high so not start pulse
    }  
    IR_Header = true;                   // mark that the start pulse was received
    ir_mask = 1;                        // set up a mask for the next bits
    return;                             // next time ISR called it will start below
  }
  delayMicroseconds(900);               // wait 900 us and test 
  if(!digitalRead(IR_PIN)) ir_string = ir_string | ir_mask;  // LOW is '1' bit (else '0' bit)
  ir_mask = ir_mask << 1;               // receiving LSB first - shift ir_mask to the left 
  ir_bit_seq++;                         // inc the bit counter
  if(ir_bit_seq == 12){                 // after remote sends 12 bits it's done
    IR_Cmnd = ir_string & B1111111;     // mask for the last 7 bits - the command
    ir_string = ir_string >> 7;         // shift the device bits over
    //IR_Dev = ir_string & B11111;        // mask for the last 5 bits - the device
    if (IR_Cmnd <= 9) {                 // Normalize digit keys from Sony remotes
      IR_Cmnd++;              					// convert raw return to actual digit values
      if (IR_Cmnd == 10) IR_Cmnd = 0;   // special case for zero key
    }
    IR_Avail = true;                    // indicate new command received
    //digitalWrite(LED_PIN, HIGH);        // turn on LED to show you got something
    ir_bit_seq = 0;                     // clean up . . .
    ir_string = 0;
    ir_mask = 0;
    IR_Header = false;
    for(int i = 0; i<25; i++){  // stop repeat! (10ms/loop) 100ms keychain - 250ms others
      delayMicroseconds(10000);         // 16383 is maximum value so need to repeat
    }
  }
}

#else    // Phillips RC5 Remote is used
void IR_ISR(){ // This ISR is called for EACH pulse of the IR sensor
  static unsigned int ir_bit_seq = 0;   // for bit number in ir_string
  static unsigned int ir_string = 0;    // stores bits received
  static byte RC5_telgr = 0;            // RC5 telegramm number
  if(RC5_telgr == 0){                 // check for first telegramm only
    while(digitalRead(IR_PIN)==1){    //skip this high period
      delayMicroseconds(10);
    }
    ir_string = 0;                            //preset result string
    delayMicroseconds(TIMERBASE_RC5/4);       //we are in fist "low", wait 1/4 bit
    for (ir_bit_seq = 0; ir_bit_seq < 14; ir_bit_seq++){  //13 bit (2 half bit are lost at start/end)
      if(digitalRead(IR_PIN)==0) ir_string++; //pin low? store: bit is "1"
      delayMicroseconds(TIMERBASE_RC5);       // wait one bit time
      ir_string = ir_string << 1;
    }
    ir_string = ir_string >> 1;
    //      Serial.println(ir_string, BIN);        //debug gerard
    IR_Cmnd = ir_string & B111111;          // mask for the last 6 bits - the command
    if(!(ir_string && 4096)) IR_Cmnd = IR_Cmnd + 64 ; // bit 13 ist command extention in RC5
    IR_Avail = true;
    //      digitalWrite(LED_PIN, HIGH);           // turn on LED to show you got something
    //      Serial.println(IR_Cmnd, DEC);        //debug gerard
    //      Serial.println(" ");                 //debug gerard
    RC5_telgr++ ;                          // telegramm counter
  }
  else{                                    //second to xth telegramm
    for(int i = 0; i<25; i++){             // stop repeat! (25ms/loop) 
      delayMicroseconds(10000);            // 16383 is maximum value so need to repeat
    }
    RC5_telgr++ ;                          // telegramm counter
    if(RC5_telgr == 3) RC5_telgr = 0;      // last (dummy) telegramm
  }
}
#endif


