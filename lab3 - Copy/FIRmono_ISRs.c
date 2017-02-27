// Welch, Wright, & Morrow, 
// Real-time Digital Signal Processing, 2011

///////////////////////////////////////////////////////////////////////
// Filename: ISRs.c
//
// Synopsis: Interrupt service routine for codec data transmit/receive
//
///////////////////////////////////////////////////////////////////////

#include "DSP_Config.h" 
#include "coeff.h"	// load the filter coefficients, B[n] ... extern
extern float convolve (float *x, const float *h, const unsigned hlen, const unsigned, const unsigned newest);
  
// Data is received as 2 16-bit words (left/right) packed into one
// 32-bit word.  The union allows the data to be accessed as a single 
// entity when transferring to and from the serial port, but still be 
// able to manipulate the left and right channels independently.

#define LEFT  0
#define RIGHT 1
#define XLEN 32
#define HLEN 31
#define NBLOCK 6
#define BUF_LEN 1<<(NBLOCK+1)

volatile union {
	Uint32 UINT;
	Int16 Channel[2];
} CodecDataIn, CodecDataOut;


/* add any global variables here */

float xLeft[XLEN];
float *pLeft = xLeft;
Int32 i;
unsigned newest = 0;

#pragma DATA_ALIGN(xLeft, BUF_LEN);

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
	float output;
//	float *p;

 	if(CheckForOverrun())					// overrun error occurred (i.e. halted DSP)
		return;								// so serial port is reset to recover

  	CodecDataIn.UINT = ReadCodecData();		// get input data samples
	
	*pLeft  = CodecDataIn.Channel[LEFT];	// store LEFT input value

	output = 0;								// set up for LEFT channel
        ++newest;
        if(newest == XLEN) newest = 0;
        xLeft[newest] = CodecDataIn.Channel[LEFT];

        output = convolve(xLeft,B,HLEN,NBLOCK,newest);

	CodecDataOut.Channel[LEFT]  = output; // store filtered value
	CodecDataOut.Channel[RIGHT] = output; // store filtered value

	WriteCodecData(CodecDataOut.UINT);		// send output data to  port
}

