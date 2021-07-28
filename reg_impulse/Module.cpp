/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// Impulse Copyright 2005 Ralph Gonzalez

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
#define PN_GATE			1
#define PN_OUT			2
#define PN_MODE			3

// mode values:
#define NORMAL			0
#define WAITFORSTATIC	1
#define ONRELEASE		2

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
    float *gateP = buffer_offset + gate_buffer;
    float *outP = buffer_offset + out_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float in=(*inP++);
		float gate=(*gateP++);
		float out=0.f;

		if (prev_gate == 0.f && gate != 0.f)
		{
			gate_on = true;
			gate_off = false;
		}
		else if (prev_gate != 0.f && gate == 0.f)
			gate_off = true;

		// Due to weird behavior in midi_to_cv, wait until
		// vel stops changing to do anything:
		// This works in mono mode but not polyphonic?
		if (mode!=WAITFORSTATIC || vel_stat==ST_STATIC)
		{
			if (gate_on)
			{
				vel = in;
				if (mode==NORMAL)
					out = vel;
				gate_on = false;
			}		
			if (gate_off)
			{
				if (mode==ONRELEASE)
					out = vel;
				// have to leave output gated up for ADSR to register
//				gate_off = false;
			}
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
	gate_on = gate_off = false;
	vel = 0.f;
	vel_stat = ST_RUN;

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
	vel_stat = getPin(PN_IN)->getStatus();
}

// describe the module
bool Module::getModuleProperties (SEModuleProperties* properties)
{
	// describe the plugin, this is the name the end-user will see.
	properties->name = "Impulse";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG Impulse";

	// Info, may include Author, Web page whatever
	properties->about = "Impulse v.1.0 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
May 2005 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
Generates impulse of height IN when GATE triggers. If mode is \n\
waitForStatic, wait for velocity input to be STATIC: useful \n\
in mono mode where velocity requires about 90 samples to settle. \n\
If mode is onRelease, then impulse occurs when GATE releases. \n";
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
	case PN_GATE:
		properties->name				= "Gate";
		properties->variable_address	= &gate_buffer;
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
	case PN_MODE:
		properties->name				= "Mode";
		properties->variable_address	= &mode;
		properties->datatype_extra		= "normal=0,waitForStatic=1,onRelease=2";
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_ENUM;
		properties->default_value		= "0";
		break;
	default:
		return false; // host will ask for plugs 0,1,2,3 etc. return false to signal when done
	};

	return true;
}

