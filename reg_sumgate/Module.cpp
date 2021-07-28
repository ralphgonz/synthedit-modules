/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// Sumgate Copyright 2005 Ralph Gonzalez

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
#define PN_GATE			0
#define PN_WIDTH		1
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
    float *gateP  = buffer_offset + gate_buffer;
    float *widthP = buffer_offset + width_buffer;
    float *outP = buffer_offset + out_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float gate=(*gateP++);
		float width=(*widthP++) * 10.f;

		if (width <= 0.f)
			width = 1.f / SAMPPERSEC;	// zero in 1 sample
		
		if (prev_gate == 0.f && gate != 0.f)
			out = 1.f;
		else if (gate != 0.f && out > 0.f)
		{
			out -= 1.f / width / SAMPPERSEC;
			if (out < 0.f)
				out = 0.f;
		}

		*outP++ = out;
		prev_gate = gate;
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

	prev_gate = 0.f;
	out = 0.f;
	SAMPPERSEC = getSampleRate();

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
	properties->name = "Sumgate";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG Sumgate";

	// Info, may include Author, Web page whatever
	properties->about = "Sumgate v.1.0.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
May 2005 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
Counts down from 1 when GATE triggers, continues counting \n\
as long as GATE is nonzero. Will reach zero after WIDTH \n\
seconds.\n\
";
	return true;
}

// describe the pins (plugs)
bool Module::getPinProperties (long index, SEPinProperties* properties)
{
	switch( index )
	{
	case PN_GATE:
		properties->name				= "Gate";
		properties->variable_address	= &gate_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_WIDTH:
		properties->name				= "Width";
		properties->variable_address	= &width_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
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

