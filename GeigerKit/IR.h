//----------------------------------------------------------------------------------------------+
//                                 Defines for IR Remote Keys
//----------------------------------------------------------------------------------------------+


// vars for IR_ISR() (must be global)
volatile boolean IR_Avail = false;      // flag set if IR has been read
volatile unsigned int IR_Cmnd = 0;      // returns IR command code received
//volatile byte IR_Dev = 0;               // returns IR device code received

// IR receiver defines . . .
#if (!IR_RC5)   // Sony Remote is used
#define ENTER     56                    // 11 or 56
#define ENTER2    11                    // 11 or 56
#define C_UP      16                    
#define C_DOWN    17                   
#define V_UP      18                  
#define V_DOWN    19                  
#define MUTE      20                    
#define POWER     21                 
#define DCML      29
#define AVTV      37
#define REC       42                 
#define RIGHT     51                 
#define LEFT      52                  
#define SLEEP     54                  
#define INFO      58                    
#define RECALL    59                   // same as INFO for the RM-EZ4 remote         
#define TEST      68                   // "Test" button on NEC remote
#define UP        116                  
#define DOWN      117               
#define KC_UP     124                  // used for RIGHT (these found on keychain menu select pad)   
#define KC_DOWN   113                  // used for LEFT
#define KC_MENU   96                   // used for ENTER
#define EXT1      255                  // not used in this IR protocol
#define PLAY      254                  // not used in this IR protocol

#else                                  // Phillips RC5 Remote is used
#define ENTER     18                   // "menu" in key chain remote
#define ENTER2    57                   // also Enter for Phillips
#define PLAY      53                   // enter key on IBM HomeDirector remotes uses PLAY code
#define C_UP      32                   // moves to the next menu option 
#define C_DOWN    33                   // moves to the next menu option
#define V_UP      16                   // increments the value already set for the current option
#define V_DOWN    17                   // decrements the value already set for the current option
#define MUTE      13                   // test communications. IR OK! displayed if IR received. 
#define POWER     12                   // enter menu system. ï¿½SEC LONG COUNTï¿½ is the first prompt
#define EXT1      56                   // finalizes the entry in the current menu option
#define AVTV      40                   // decimal point
#define RECALL    55                   // not implemented in key chain remote
#define TEST      68                   // "Test" button on NEC remote - only here for compile
#define RIGHT     46                   // increments the value already set for the current option
#define LEFT      45                   // decrements the value already set for the current option
#define SLEEP     34                   // curved arrows in key chain remote              
#define INFO      10                   // --/- in key chain remote, toggle backlight                   
#define UP        43                  
#define DOWN      44   
#define KC_UP     100  // not used yet // for RIGHT (these found on keychain menu select pad)   
#define KC_DOWN   101                  // for LEFT
#define KC_MENU   102                  // for ENTER
#define DCML      255                  // not used in this IR protocol
#endif

