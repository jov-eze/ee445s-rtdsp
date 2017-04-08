// Welch, Wright, & Morrow, 
// Real-time Digital Signal Processing, 2011

///////////////////////////////////////////////////////////////////////
// Filename: impulseModulatedBPSK_ISRs.c
//
// Synopsis: Interrupt service routine for codec data transmit/receive
//
///////////////////////////////////////////////////////////////////////

#include "DSP_Config.h" 
#include <stdlib.h>	// needed to call the rand() function
  
// Data is received as 2 16-bit words (left/right) packed into one
// 32-bit word.  The union allows the data to be accessed as a single 
// entity when transferring to and from the serial port, but still be 
// able to manipulate the left and right channels independently.

#define LEFT  0
#define RIGHT 1

volatile union {
	Uint32 UINT;
	Int16 Channel[2];
} CodecDataIn, CodecDataOut;


/******		PART 2 IIR BUTTERWORTH FILTERS		******/

// from Lab 3, Part 4-6
#define N 6		// IIR filter order
float A[N+1] = { 1, 0, -1, 1, -1.96004314890818, 0.984414127416097};  // numerator coefficients
float Ag[2] = {0.00779293629195165, 1};
float B[N+1] = { 1, 0, -1, 1, -1.87292545631220, 0.969067417193792}; // denom coefficients
float Bg[2] = {0.0154662914031040, 1};
//float x[N+1] = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};   // input value (buffered)
//float y[N+1] = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};   // output values (buffered)
float *currentInput = &x[0];

// intial globals
Int32 counter = 0;
Int32 samplesPerSymbol = 20;
Int32 symbol;
Int32 data[2] = {-15000, 15000};
Int32 cosine[4] = {1, 0, -1, 0};
Int32 scrambleInit=5;
Int32 i;

float x[8];
float y;
float output;


void iir_df_one(){
	//y[n] = sum(bk*x) - sum(ak*y)

	float sumB = 0;
	float sumA = 0;
	int k;
	for (k = 0; k<=N; k++){
	    sumB += B[k]*x[k];
	}
	for(k=1; k<=N; k++){
	    sumA += A[k]*y[k];
	}


	float currentY = sumB-sumA;
	float currentX = currentInput[0];

	y[0] = currentY;
	x[0] = currentX;

	for(k=N; k>0; k--){
	    y[k] = y[k-1];
	    x[k] = x[k-1];
	}
}

float clock_recover(float y){
	// PART 2 FUNCTION
	float clk;
		clk = iir_df_one(i);			// Prefilter B(w)
		clk *= outPut;					// Squarer
		clk = iir_df_one(i);			// Bandpass Filter H(w)
	return clk;

}

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
{	/* add any local variables here */

 	if(CheckForOverrun())					// overrun error occurred (i.e. halted DSP)
		return;								// so serial port is reset to recover
  	CodecDataIn.UINT = ReadCodecData();		// get input data samples

	/* add your code starting here */
    if (counter == 0) {
		symbol = rand() & 1; // a faster version of rand() % 2
		symbol = scramble(&scrambleInit, 1);
		xO[0] = data[symbol]; // read the table
	}

    // perform impulse modulation based on the FIR filter, B[N]
    y  = 0;

    for (i = 0; i < 8; i++) {
		y +=  x[i]*B[counter + 20*i];	// perform the dot-product
	}

    if (counter == (samplesPerSymbol - 1)) {
    	counter = -1;

		/* shift x[] in preparation for the next symbol */
 		for (i = 7; i > 0; i--) {
			x[i] = x[i - 1];          // setup x[] for the next input
		}
   	}

   	counter++;

	output = y; //*cosine[counter & 3];
	float clk = 15000*clock_recover(y/15000);

	CodecDataOut.Channel[LEFT]  = y; // setup the LEFT  value
	CodecDataOut.Channel[RIGHT] = clk; // setup the RIGHT value
	/* end your code here */

	WriteCodecData(CodecDataOut.UINT);		// send output data to  port
}

