// Welch, Wright, & Morrow, 
// Real-time Digital Signal Processing, 2011

///////////////////////////////////////////////////////////////////////
// Filename: impulseModulatedBPSK_ISRs.c
//
// Synopsis: Interrupt service routine for codec data transmit/receive
//
///////////////////////////////////////////////////////////////////////

#include "DSP_Config.h" 
#include "coeff.h"
#include <stdlib.h>	// needed to call the rand() function
  
// Data is received as 2 16-bit words (left/right) packed into one
// 32-bit word.  The union allows the data to be accessed as a single 
// entity when transferring to and from the serial port, but still be 
// able to manipulate the left and right channels independently.

#define LEFT  0
#define RIGHT 1

float clk_sample[100];
int i_sample = 0;

volatile union {
	Uint32 UINT;
	Int16 Channel[2];
} CodecDataIn, CodecDataOut;

/******		PART 2 IIR BUTTERWORTH FILTERS		******/

// from Lab 3, Part 4-6
float preB[3] = { 1, 0, -1} ; //numertor  [num den]
float preA[3] = {1, -1.96004314890818, 0.984414127416097};  // numerator coefficients
float pregain = 0.00779293629195165;
float prey[3] = {0.0, 0.0, 0.0} ;
float prex[3] = {0.0, 0.0, 0.0} ;

float bandB[3] = { 1, 0, -1};
float bandA[3] = {1, -1.87292545631220, 0.969067417193792}; // denom coefficients
float bandgain = 0.0154662914031040 ;
float bandy[3] = {0.0, 0.0, 0.0} ;
float bandx[3] = {0.0, 0.0, 0.0} ;

/******		PART 1 		******/
float x[8];
float y;

Int32 counter = 0;
Int32 samplesPerSymbol = 20;
Int32 symbol = 0;
Int32 data[2] = {-15000, 15000};
Int32 cosine[4] = {1, 0, -1, 0};
Int32 scrambleInit=5;
Int32 i;
float output;

/******		PART 2 FUNCTIONS		******/
float bpf(float input){
	//y[n] = sum(bk*x) - sum(ak*y)

	float sumB = 0;
	float sumA = 0;
	int k;
	for (k = 0; k<=N; k++){
	    sumB += bandB[k]*bandx[k];
	}
	for(k=1; k<=N; k++){
	    sumA += bandA[k]*bandy[k];
	}


	float currentY = sumB-sumA;
	// float currentX = bandgain*x[0];
	float currentX = bandgain*input

	bandy[0] = currentY;
	bandx[0] = currentX;

	for(k=N; k>0; k--){
	   bandy[k] = bandy[k-1];
	   bandx[k] = bandx[k-1];
	}

	return bandy[0];
}


float prefilter(float input){
	//y[n] = sum(bk*x) - sum(ak*y)

	float sumB = 0;
	float sumA = 0;
	int k;
	for (k = 0; k<=N; k++){
	    sumB += preB[k]*prex[k];
	}
	for(k=1; k<=N; k++){
	    sumA += preA[k]*prey[k];
	}


	float currentY = sumB-sumA;
	// float currentX = pregain*prex[0];
	float currentX = pregain*input;

	prey[0] = currentY;
	prex[0] = currentX;

	for(k=N; k>0; k--){
		prey[k] = prey[k-1];
		prex[k] = prex[k-1];
	}

	return prey[0];
}

float clock_recover(float input){
/*	may have to switch order of prefilter and bpf	*/
		float clk = prefilter(input);	// Prefilter B(w)
		clk = clk*clk;			// Squarer
		clk = bpf(clk);			// Bandpass Filter H(w)
	return clk;

}

/******		PART 1 FUNCTIONS		******/
int scramble(int* symbol_ptr, int inBit){
	int symbol, y23, y18, yn;
	symbol = *symbol_ptr & 0x840000;
	y23 = (symbol & 0x800000) >> 23;
	y18 = (symbol & 0x40000) >> 18 ;
	yn = y23 ^ y18 ^ inBit;
	*symbol_ptr = (*symbol_ptr >> 1) | (yn << 30);

	return yn;
}

interrupt void Codec_ISR()
/**********************************************************************
// Purpose:   Codec interface interrupt service routine
//
// Input:     None
//
// Returns:   Nothing
//
// Calls:     CheckForOverrun, ReadCodecData, WriteCodecData
//
// Notes:     None
**********************************************************************/

{	/* add any local variables here */

 	if(CheckForOverrun())					// overrun error occurred (i.e. halted DSP)
		return;								// so serial port is reset to recover
  	CodecDataIn.UINT = ReadCodecData();		// get input data samples

/********* 			SCRAMBLE BIT GENERATOR			 *********/
	if (counter == 0){
		symbol %= 2;
		symbol += 1;
		// symbol = 1000*scramble(&scrambleInit, 1);
		x[0] = data[symbol];
	}

/********* 			PART ONE			 *********/
	y  = 0;
    for (i = 0; i < 8; i++) {
		y +=  x[i]*B[counter + 20*i];	// perform the dot-product
	}
    if (counter == (samplesPerSymbol - 1)) {
    	counter = -1;

		//shift x[] in preparation for the next symbol
 		for (i = 7; i > 0; i--) {
			x[i] = x[i - 1];        // setup x[] for the next input
		}
   	}
	counter++;						// if counter > 0, won't generate a random symbol
	output = y;						//*cosine[counter & 3];

/********* 			SYMBOL RECOVERY			 *********/
	float clk = clock_recover(x[0]/15000);
	if(i_sample < 100){
		clk_sample[i_sample] = clk;
		i_sample += 1;
	}

/********* 			OUTPUT TO BOARD			 *********/
	CodecDataOut.Channel[LEFT]  = y; // setup the LEFT  value
	CodecDataOut.Channel[RIGHT] = 15000*clk; // setup the RIGHT value

	WriteCodecData(CodecDataOut.UINT);		// send output data to  port

}
