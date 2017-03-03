// Welch, Wright, & Morrow, 
// Real-time Digital Signal Processing, 2011

///////////////////////////////////////////////////////////////////////
// Filename: ISRs.c
//
// Synopsis: Interrupt service routine for codec data transmit/receive
//
///////////////////////////////////////////////////////////////////////

#include "DSP_Config.h" 
  
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


/* add any global variables here */
#define N 6		// IIR filter order

float B[N+1] = { 0.108499, 0, -0.325498, 0, 0.325498, 0, -0.108499};  // numerator coefficients
float A[N+1] = {1, -1.13553, 0.944316, -0.637821, 0.544417, -0.173657, 0.0459008}; // denom coefficients
float x[N+1] = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};   // input value (buffered)
float y[N+1] = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};   // output values (buffered)
float *currentInput = &x[0];

void iir_df_one(){
//	CodecDataIn.UINT = ReadCodecData();
//	*currentInput = CodecDataIn.Channel[0];
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
	
	/* I added my IIR filter routine here */
//	x[0] = CodecDataIn.Channel[LEFT];		// current input value
	x[0] = CodecDataIn.Channel[RIGHT];		// current input value


	iir_df_one();
	
	CodecDataOut.Channel[LEFT]  = y[0];		// setup the LEFT value	
	CodecDataOut.Channel[RIGHT]=x[0];
	/* end of my IIR filter routine	*/

	WriteCodecData(CodecDataOut.UINT);		// send output data to  port
}

