// Welch, Wright, & Morrow, 
// Real-time Digital Signal Processing, 2011

///////////////////////////////////////////////////////////////////////
// Filename: ISRs.c
//
// Synopsis: Interrupt service routine for codec data transmit/receive
//
///////////////////////////////////////////////////////////////////////

#include "DSP_Config.h" 
#include <stdlib.h>
// Data is received as 2 16-bit words (left/right) packed into one
// 32-bit word.  The union allows the data to be accessed as a single 
// entity when transferring to and from the serial port, but still be 
// able to manipulate the left and right channels independently.

#define LEFT  	0
#define RIGHT 	1
#define G 		32000
#define input 	0;

volatile union {
	Uint32 UINT;
	Int16 Channel[2];
} CodecDataIn, CodecDataOut;

/* add any global variables here */
int PNSequence;
int SSRG_state = 13;
int* state_ptr = &SSRG_state;

int dsVar;
int ddVar;
int index = 0;
int DD_DS[100];
int DS[100];
int DD[100];

int state_index  = 0;

//4.2 vars
int DSstate = 15;		// only care about last 5 bits
int DDstate = 15;

int random(int min, int max){
	int range = max - min +1;
	return (rand()% range + min);
}

int updateSSRG_state(int* state_ptr, int inBit){
	int state, y2, y5, yn;

	state = *state_ptr & 31;
	y2 = state & 8;
	y2 = y2 >> 3;
	y5 = state & 1;
	yn = y2 ^ y5 ^ inBit;

	*state_ptr = (*state_ptr >> 1) | (yn << 4);

	return yn;
}

//4.1 methods
int scramble(int* state_ptr, int inBit){
	int state, y2, y5, yn;

	state = *state_ptr & 31;
	y2 = state & 8;
	y2 = y2 >> 3;
	y5 = state & 1;
	yn = y2 ^ y5 ^ inBit;

	*state_ptr = (*state_ptr >> 1) | (yn << 4);

	return yn;
}

int deScramble(int* state_ptr, int inBit){
	int state, y2, y5, yn;

	state = *state_ptr & 31;
	y2 = state & 8;
	y2 = y2 >> 3;
	y5 = state & 1;
	yn = y2 ^ y5 ^ inBit;

	*state_ptr = (*state_ptr >> 1) | (inBit << 4);

	return yn;
}
//4.2 methods

interrupt void Codec_ISR()
///////////////////////////////////////////////////////////////////////
// Purpose:   Codec interface interrupt service routine  
//
// Input:     None
//
// Returns:   Nothing
//
// Calls:     CheckForOverrun, ReadCodecData, WriteCodecData
//
// Notes:     None
///////////////////////////////////////////////////////////////////////
{                    
	/* add any local variables here */
 	if(CheckForOverrun())					// overrun error occurred (i.e. halted DSP)
		return;								// so serial port is reset to recover

  	CodecDataIn.UINT = ReadCodecData();		// get input data samples
	
//	dsVar = scramble(&DSstate, 0);
//	ddVar = deScramble(&DDstate, dsVar);
//	if (index < 100){
//		DD[index] = DDstate;
//		DS[index] = DSstate;
//		DD_DS[index] = ddVar;
//		index += 1;
//	}

  	updateSSRG_state(state_ptr, 0);


	CodecDataOut.Channel[1] = 32000 * ddVar; 		// L to R
	CodecDataOut.Channel[0]  = 32000*1;			 // temp to L
	WriteCodecData(CodecDataOut.UINT);		// send output data to  port
}

// #define LEFT  	0
// #define RIGHT 	1
