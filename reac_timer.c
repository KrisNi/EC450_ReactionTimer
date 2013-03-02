#include  "msp430g2553.h"
// define bit masks for the 3 I/O channels that we use
#define RED 1
#define GREEN 64
#define BUTTON 8

/* ===== GLOBAL VARIABLES =====
 * These variables are shared between the main program and the WDT
 * interrupt handler and represent the 'state' of the application.
 * blink_interval is made a variable rather than a constant so we can
 * change it in the debugger.
 */

unsigned int count; //count for the LED blinks
unsigned int timing; //reaction time = timing * 7.4ms
unsigned int trigger; //trigger to start the timing
unsigned int mode; //switch between different button pushes
volatile unsigned int blink_counter;   // down counter for interrupt handler
unsigned char last_button_state;   // state of the button the last time it was read

// ===== Main Program (System Reset Handler) =====
void main(void)
{
	BCSCTL1 = CALBC1_1MHZ;	// 1 Mhz calibration for clock initially
	DCOCTL  = CALDCO_1MHZ;
  // setup the watchdog timer as an interval timer
  WDTCTL =(WDTPW + // (bits 15-8) password
                   // bit 7=0 => watchdog timer on
                   // bit 6=0 => NMI on rising edge (not used here)
                   // bit 5=0 => RST/NMI pin does a reset (not used here)
           WDTTMSEL + // (bit 4) select interval timer mode
           WDTCNTCL +  // (bit 3) clear watchdog timer counter
  		          0 // bit 2=0 => SMCLK is the source
  		          +1 // bits 1-0 = 01 => source/8K ~ 134 Hz <-> 7.4ms.
  		   );
  IE1 |= WDTIE;		// enable the WDT interrupt (in the system interrupt register IE1)

  // initialize the I/O port.  Pins 0-1-2 are used
  P1DIR = RED + GREEN;     // Set P1.0 and P1.6, P1.3 is input (the default)
  P1REN = BUTTON;  // enable internal 'PULL' resistor for the button
  P1OUT = BUTTON;  // pull up

  // initialize the state variables
  blink_counter=0;// corresponds to about 1/2 sec
  count = 6; //count equals 6 ensures the program to run multiple times
  timing = 0;
  trigger = 0;
  mode = 0; //0 means its waiting for the fisrt button push
  last_button_state=(P1IN&BUTTON);

 // in the CPU status register, set bits:
 //    GIE == CPU general interrupt enable
 //    LPM0_bits == bit to set for CPUOFF (which is low power mode 0)
 _bis_SR_register(GIE+LPM0_bits);  // after this instruction, the CPU is off!
}

// ===== Watchdog Timer Interrupt Handler =====
// This event handler is called to handle the watchdog timer interrupt,
//    which is occurring regularly at intervals of 8K/1.1MHz ~= 7.4ms.

interrupt void WDT_interval_handler(){
  char current_button;            // variable to hold the state of the button
  // poll the button to see if we need to change state
  current_button=(P1IN & BUTTON); // read button bit

  if ((current_button==0) && last_button_state && !mode && count > 5){ // did the button go down?
	  mode ^= 1; //switch between two button pushes mode
	  blink_counter=68;
	  timing = 0;
	  count = 0;
  }
  else if ((current_button==0) && last_button_state && mode && count > 5) {
	  mode ^= 1; //switch between two button pushes mode
  	  trigger = 0; //trigger the timing counter to increase
  }

  //decrese the counter and make the red LED blink
  if (--blink_counter==0){
	  switch (count){
	  case 0:
	  case 1:
	  case 2:
	  case 3: P1OUT ^= GREEN;  //first four cases are the first two green LED blinks
	  	 	  blink_counter=68;
	  	 	  count++;
	  	 	  break;
	  case 4: P1OUT ^= RED;   //followed by a red LED blink
	  	 	  blink_counter=68;
	  	 	  count++;
	  	 	  break;
	  case 5: P1OUT ^= RED;
	  	  	  blink_counter=68;
	  	  	  trigger = 1;  //when red LED goes off, triggers the timing counter starting to increase
	  	  	  count++;
	  	  	  break;
	  default:
		  	  blink_counter=0;
		  	  break;
	  }
  }

  if (trigger) {
	  timing++; //if the trigger is set to 1, increment the timing counter
  }

  last_button_state=current_button;
}

// DECLARE function WDT_interval_handler as handler for interrupt 10
// using a macro defined in the msp430g2553.h include file
ISR_VECTOR(WDT_interval_handler, ".int10")

