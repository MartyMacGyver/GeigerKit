//----------------------------------------------------------------------------------------------+
//                            IR Functions for NEC remote
//     ALL protocol described here - http://www.sbprojects.com/knowledge/ir/index.php
//----------------------------------------------------------------------------------------------+


void IR_ISR(){ // This ISR is called for EACH pulse of the IR sensor
  static boolean IR_Header = false;     // flag set if IR header was received
  static boolean IR_Space = false;      // flag set if 4.5ms space after header was received - if NEC
  static unsigned long timer;           // for timing distance - if NEC
  static unsigned long distance;        // for timing distance - if NEC
  static unsigned long ir_string = 0;   // stores bits received - must be 32 bits for NEC
  static unsigned long ir_mask = 0;     // for masking ir_string 
  static unsigned int ir_bit_seq = 0;   // for bit number in ir_string

  if(IR_Header == false){               // check for the long start pulse
    for(int i = 0; i<80; i++){          // see if it lasts at least 8 ms
      delayMicroseconds(100);           // delay in chunks to get out ASAP
      if(digitalRead(IR_PIN)) return;   // low active and it went high so not start pulse
    }  
    // header is good!
    IR_Header = true;                   // mark that the start pulse was received
    ir_mask = 1;                        // set up a mask for the next bits
    timer = micros();                   // this is pulse DISTANCE modulation - start a timer          
    return;                             // next time ISR called it will start below
  }

  // we are here because we received a start header already
  distance = micros() - timer;          // get the distance from the last interrupt

  if (distance > 4000){                 // looking for 4.5ms space after header
    IR_Space = true;                    // mark that the space was received
    timer = micros();                   // start the timer again         
    return;                             // next time ISR called it will start below
  }

  if (IR_Space){
    if (distance > 3000){               // no pulse in > 3 mS it's an error - start over
      ir_bit_seq = 0;                   // clean up . . .
      ir_string = 0;
      ir_mask = 0;
      IR_Header = false;
      IR_Space = false;
      return;
    }
    // header and space after were good - start getting the bits . . .
    if (distance > 1500){               // > 1.5 ms you got a 1
      ir_string = ir_string | ir_mask;  // put a 1 in the string
    }
    // if the pulse was < 1.5mS you got a 0 - nothing to do since we shift the mask which adds the zero
    ir_mask = ir_mask << 1;             // receiving LSB first - shift ir_mask to the left 
    ir_bit_seq++;                       // inc the bit counter
    timer = micros();                   // reset the timer for the next bit


    if(ir_bit_seq == 32){               // after remote sends 32 bits it's done
      // now fish out the data - the bits are in the right order but the bytes are reversed
      // The 4 bytes from MSB -> LSB are: INV command, command, INV address, address

      //IR_Dev = ir_string & B11111111;   // mask the LSB 8 bits - the address - always 0 & NOT USED
      ir_string = ir_string >> 16;      // shift out till the second byte
      IR_Cmnd = ir_string & B11111111;  // mask what is now the right 8 bits - the raw command
      IR_Cmnd = Normalize (IR_Cmnd);    // normalize to Sony command codes - comment out to get raw

      IR_Avail = true;                  // indicate new command received
      digitalWrite(LED_PIN, HIGH);      // turn on LED 
      ir_bit_seq = 0;                   // clean up . . .
      ir_string = 0;
      ir_mask = 0;
      IR_Header = false;
      IR_Space = false;
    }
  }
}


unsigned int Normalize(byte cmndIn){  // convert commands from NEC IR to the same commands as Sony
  byte idx;

  // inArray is in order of keys on the remote - top left to bottom right
  byte inArray[21] = {
    69, 70, 71, 68, 64, 67, 7, 21, 9, 22, 25, 13, 12, 24, 94, 8, 28, 90, 66, 82, 74};
  // outArray maps to functions
  byte outArray[21] = {
    POWER, 0, MENU, TEST, PLUS, BACK, L_ARROW, ENTER, R_ARROW, 0, MINUS, CLEAR, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  for (idx = 0; idx<22; idx++){
    if (cmndIn == inArray[idx]) break;
  }
  return outArray[idx];
}




