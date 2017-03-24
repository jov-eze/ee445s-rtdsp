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

#define LEFT  0
#define RIGHT 1
#define G 32000

volatile union {
	Uint32 UINT;
	Int16 Channel[2];
} CodecDataIn, CodecDataOut;

/* add any global variables here */
int PNSequence;
int SSRG_state = 13;
int* state_ptr = &SSRG_state;
int inputBit = 0;
int yn = 0;
int var;

int randVal[100];
int r_index = 0;
int a[100];
int a_index = 0;

//4.2 vars
int DSstate;
int DDstate;
//4.2 vars

int random(int min, int max){
	int range = max - min +1;
	return (rand()% range + min);
}


int updateSSRG_State(int* state_ptr, int inBit){
	int state, y2, y5, yn;

	state = *state_ptr & 31;
	y2 = state & 8;
	y2 = y2 >> 3;
	y5 = state & 1;
	yn = y2 ^ y5 ^ 0;

	*state_ptr = (state >> 1) | (yn << 4);

	if(a_index < 100){
		a[a_index] = *state_ptr;
		a_index += 1;
	}
	return yn;
}

//4.2 methods

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
	
//	*state_ptr = random(1,31);
	/*if(r_index < 100){
		randVal[r_index] = *state_ptr;
		r_index += 1;
	}*/
	var = updateSSRG_State(state_ptr, 0);

	CodecDataOut.Channel[RIGHT] = 32000 * ((SSRG_state >> 4) & 1); // L to R
//	CodecDataOut.Channel[LEFT]  = 0;			 // temp to L
	WriteCodecData(CodecDataOut.UINT);		// send output data to  port
}

