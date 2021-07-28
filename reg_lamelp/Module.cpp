/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// MovAvg Copyright 2005 Ralph Gonzalez

#include "Module.h"
#include "SEMod_struct.h"
#include "SEPin.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>

//#define DDEBUG
#ifdef DDEBUG
#include <stdio.h>
static FILE* fp=NULL;
#endif

// instead of refering to the pins by number, give them all a name
#define PN_IN			0
#define PN_MALENGTH		1
#define PN_OUT			2

// handy function to fix denormals.
static const float anti_denormal = 1e-18f;
inline void kill_denormal(float &p_sample ) { p_sample += anti_denormal;p_sample -= anti_denormal;};

Module::Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1) : SEModule_base(seaudioMaster, p_resvd1)
{
	maBuf = NULL;
}

Module::~Module()
{
}

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *inP  = buffer_offset + in_buffer;
	float *maLengthP = buffer_offset + maLength_buffer;
    float *outP = buffer_offset + out_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float in=(*inP++);
		float maLength=MAMAX * (*maLengthP++);

		*outP++ = movAvg(in,(int)(maLength+.5f),maLengthPrev,maCount,maBuf,maSum); // cheap LP filter
	}
}

void Module::close()
{
	free(maBuf);
	SEModule_base::close();

#ifdef DDEBUG
if (fp!=NULL)
  fclose(fp);
#endif
}

// perform any initialisation here
void Module::open()
{
#ifdef DDEBUG
fp = fopen("xxx","w");
if (fp==NULL)
  exit(1);
#endif

	maSum = 0.f;
	maCount = 0;
	maLengthPrev = 0;

	// 1000 samples at 44.1k sample rate: min cutoff around 20 Hz
	SAMPPERSEC = getSampleRate();
	MAMAX = (int)(SAMPPERSEC/44 + 1);
	maBuf = (float*)malloc((sizeof(float)) * MAMAX);
	memset(maBuf, 0, MAMAX * sizeof(float) );


	// this macro switches the active processing function
	// e.g you may have several versions of your code, optimised for a certain situation
	SET_PROCESS_FUNC(Module::sub_process);

	SEModule_base::open(); // call the base class

	// let 'downstream' modules know audio data is coming
	getPin(PN_OUT)->TransmitStatusChange( SampleClock(), ST_RUN );
}


// this routine is called whenever an input changes status.
// e.g when the user changes a module's parameters,
// or when audio stops/starts streaming into a pin
void Module::OnPlugStateChange(SEPin *pin)
{
}

// describe the module
bool Module::getModuleProperties (SEModuleProperties* properties)
{
	// describe the plugin, this is the name the end-user will see.
	properties->name = "MovAvg";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG MovAvg";

	// Info, may include Author, Web page whatever
	properties->about = "MovAvg v.1.0.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
Jan 2005 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
Moving average of input, or rudimentary 'boxcar' low-pass digital filter. \n\
";
	return true;
}

// describe the pins (plugs)
bool Module::getPinProperties (long index, SEPinProperties* properties)
{
	switch( index )
	{
	case PN_IN:
		properties->name				= "In";
		properties->variable_address	= &in_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		properties->flags				= IO_LINEAR_INPUT;
		break;
	case PN_MALENGTH:
		properties->name				= "Width";
		properties->variable_address	= &maLength_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		properties->datatype_extra		= "10,0,10,0";
		break;
	case PN_OUT:
		properties->name				= "Out";
		properties->variable_address	= &out_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	default:
		return false; // host will ask for plugs 0,1,2,3 etc. return false to signal when done
	};

	return true;
}

// mod function -- C's % operator doesn't do neg like perl does
//#define MOD(i,n) ((i)>=0 ? (i)%(n) : (i)+(n))
#define MOD(i,n) ((i)>=0 ? (i) : (i)+(n))
 
// Highly efficient "boxcar" digital filter, simply
// the moving average of last n samples. This has a lot
// of stopband variation, but ok in this application where
// the output is a comb filter anyway.
// Ring-buffer contains history.
//
// To translate cutoff frequency to n samples:
//   n = SAMPPERSEC/(2*f); if (n<=1) ...
//
float Module::movAvg(float x, int n, int& maLengthPrev, int& maCount, float maBuf[], float& maSum)
{

	maBuf[maCount] = x;

	if (n<=1)
		maSum = x;
	else if (n != maLengthPrev)
	{
		maSum = 0.;
		for (int i=0 ; i<n ; ++i)
			maSum += maBuf[MOD(maCount-i,MAMAX)];
	}
	else
	{
		maSum += maBuf[maCount];
		maSum -= maBuf[MOD(maCount-n,MAMAX)];
		maSum *= .9999f;	// for stability
	}

#ifdef DDEBUG
	fprintf(fp,"%d %d\n",
		maCount,(maCount-n) % MAMAX);
#endif
	++maCount;
	maCount %= MAMAX;
	maLengthPrev = n;

	return (n<=1 ? maSum : maSum/n);
}
