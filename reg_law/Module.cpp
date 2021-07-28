/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// Law Copyright 2005 Ralph Gonzalez

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
#define PN_LAW			1
#define PN_OUT			2

// handy function to fix denormals.
static const float anti_denormal = 1e-18f;
inline void kill_denormal(float &p_sample ) { p_sample += anti_denormal;p_sample -= anti_denormal;};

Module::Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1) : SEModule_base(seaudioMaster, p_resvd1)
{
}

Module::~Module()
{
}

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *inP  = buffer_offset + in_buffer;
    float *lawP = buffer_offset + law_buffer;
    float *outP = buffer_offset + out_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float in=(*inP++);
		float law=(*lawP++);

		// TODO: if cpu is really an issue then create
		// a table for each integral value of law
		// and interpolate for 'in' values... for
		// 'economy' mode operation.

		// clip input to avoid huge output:
		if (in > 1.f)
			in = 1.f;
		else if (in < 0.f)
			in = 0.f;

		if (law != prev_law)	// update derived law
		{
			prev_law = law;
			y = (float)pow(10.f, law);
		}

		// to reduce cpu:
		if (in==prev_in && y==prev_y)
			*outP++ = prev_out;
		else
		{
			*outP++ = prev_out = (float)pow(in, y);
			prev_y = y;
			prev_in = in;
		}
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

	prev_law = 0.f;
	prev_in = -1.f;
	prev_y = -1.f;
	prev_out = 0.f;
	y = 1.f;	// = 10 ^ 0

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
	getPin(PN_OUT)->TransmitStatusChange( SampleClock(), getPin(PN_IN)->getStatus() );
}

// describe the module
bool Module::getModuleProperties (SEModuleProperties* properties)
{
	// describe the plugin, this is the name the end-user will see.
	properties->name = "Law";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG Law";

	// Info, may include Author, Web page whatever
	properties->about = "Law v.1.0 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
May 2005 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
Applies gain LAW to IN. LAW selects an exponent. If LAW is 0 \n\
the exponent is 1, corresponding to a conventional linear law. \n\
Negative LAWs correspond to fractional exponents (roots), with \n\
the effect that output rises quickly for small inputs and \n\
remains fairly constant for larger inputs. Positive LAWs \n\
correspond to powers greater than 1, so that output rises \n\
slowly for small inputs and then increases quickly as IN \n\
approaches 10. LAW=-3 corresponds to a square root and LAW=3 \n\
corresponds to a square law. A value of 5 gives a similar curve \n\
to the VCA module in exponential or decibel mode. \n\
\n\
Assumes input is between 0 and 10, returns value between 0 and 10,\n\
clipping input as neccessary to avoid large output values.\n\
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
		break;
	case PN_LAW:
		properties->name				= "Law";
		properties->variable_address	= &law_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		properties->datatype_extra		= "10,-10,10,-10";
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

