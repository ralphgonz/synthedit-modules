/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// Static Copyright 2004 Ralph Gonzalez

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
#define PN_TAU			0
#define PN_ALPHA		1
#define PN_BETA			2
#define PN_OUT			3
 
Module::Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1) : SEModule_base(seaudioMaster, p_resvd1)
{
}

Module::~Module()
{
}

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *alphaP  = buffer_offset + alpha_buffer;
    float *betaP  = buffer_offset + beta_buffer;
    float *tauP  = buffer_offset + tau_buffer;
    float *outP = buffer_offset + out_buffer;
	for( int s = sampleFrames; s > 0 ; s-- )
	{
		double alpha=*alphaP++;
		double beta=(*betaP++) * 10.F;
		double tau=*tauP++;

		// TODO: i want this to give a variable response curve
		// with 0:straight line x=y, 1:lower right angle,
		// -1: upper right angle, intermediate values give
		// parabola (?) with varying degrees of sharpness.

		float out=0.;

		*outP++ = out;

#ifdef DDEBUG
fprintf(fp,"%d %d %d %d %.3f\n",
	pmCount,pmPeriod,pos2,transPeriod,pmDepth);
#endif
	}
}

void Module::close()
{
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
	properties->name = "ResponseCurve";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG ResponseCurve";

	// Info, may include Author, Web page whatever
	properties->about = "Response Curve v.1.0 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
May 2004 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
";
	return true;
}

// describe the pins (plugs)
bool Module::getPinProperties (long index, SEPinProperties* properties)
{
	switch( index )
	{
	case PN_TAU:
		properties->name				= "Gain";
		properties->variable_address	= &tau_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_ALPHA:
		properties->name				= "Range";
		properties->variable_address	= &alpha_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_BETA:
		properties->name				= "Sparse";
		properties->variable_address	= &beta_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "7";
		properties->datatype_extra		= "15,0,15,0";
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

