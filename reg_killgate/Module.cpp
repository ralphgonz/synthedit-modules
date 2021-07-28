/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// Killgate Copyright 2003 Ralph Gonzalez

/*
*/

#include "Module.h"
#include "SEMod_struct.h"
#include "SEPin.h"
#include <assert.h>
#include <stdlib.h>

// instead of refering to the pins by number, give them all a name
#define PN_INPUT	0
#define PN_GATE		1
#define PN_OUTPUT	2
#define PN_KILLVAL	3
#define PN_MODE		4

Module::Module(seaudioMasterCallback seaudioMaster) : SEModule_base(seaudioMaster)
{
}

Module::~Module()
{
}

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *inP  = buffer_offset + input_buffer;
    float *gateP  = buffer_offset + gate_buffer;
    float *outputP = buffer_offset + output_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float g=*gateP++;
		float in=*inP++;
		bool selfGate=(!getPin(PN_GATE)->IsConnected());
		if (selfGate)
			g = in;
		bool current_gate = (g != 0.);
		float out=in;
		
		if (mode==1 && gate_open)
			out = killval;

		if (!gate_open && current_gate)
		{
			gate_open = true;
			if (mode==2)
				out = killval;
		}
		else if (gate_open && !current_gate)
			gate_open = false;

		if (mode==0 && gate_open)
			out = killval;

//		// tell downstream modules whether I am changing,
//		// since this happens infrequently
//		if (out != prevOut)
//		{
//			prevOut = out;
//			outIsStatic = false;
//			getPin(PN_OUTPUT)->TransmitStatusChange( SampleClock(), ST_RUN);
//		}
//		else if (!outIsStatic)
//		{
//			outIsStatic = true;
//			getPin(PN_OUTPUT)->TransmitStatusChange( SampleClock(), ST_STATIC);
//		}
		*outputP++ = out;
	}
}

void Module::close()
{
	SEModule_base::close();
}

// perform any initialisation here
void Module::open()
{
	// this macro switches the active processing function
	// e.g you may have several versions of your code, optimised for a certain situation
	SET_PROCESS_FUNC(Module::sub_process);

	SEModule_base::open(); // call the base class

	gate_open = false;
//	outIsStatic = false;
//	prevOut = -9999.f;

	// let 'downstream' modules know audio data is coming
	getPin(PN_OUTPUT)->TransmitStatusChange( SampleClock(), ST_RUN );
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
	properties->name = "Kill Gate";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG Kill Gate";

	// Info, may include Author, Web page whatever
	properties->about = "Kill Gate v.1.0.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
July 2003 \n\
 \n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
 \n\
This is a little hack which I've found handy though you can doubtless \n\
get the same effect from combining other modules: it simply outputs \n\
its INPUT (1 by default) until a signal appears on the GATE. Then \n\
it outputs the KILL VALUE (0 by default). In 'latch' MODE (default) \n\
the kill value remains until the gate closes. In 'latchdelay' MODE \n\
the kill value is delayed by 1 sample. In 'onesample' MODE the \n\
kill value is only output for one sample when the gate opens. \n\
 \n\
One use is to feed the RELEASE inputs on secondary Env Seg \n\
modules, so that retriggering the primary Env Seg will instantly \n\
release all secondaries. In this case you use the 'onesample' MODE, \n\
and apply the main gate signal to the Killgate's GATE input. \n\
";
	return true;
}

// describe the pins (plugs)
bool Module::getPinProperties (long index, SEPinProperties* properties)
{
	switch( index )
	{
	case PN_INPUT:
		properties->name				= "in";
		properties->variable_address	= &input_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "1";
		break;
	case PN_GATE:
		properties->name				= "gate";
		properties->variable_address	= &gate_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_OUTPUT:
		properties->name				= "Output";
		properties->variable_address	= &output_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case PN_KILLVAL:
		properties->name				= "Kill value";
		properties->variable_address	= &killval;
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_FLOAT;
		properties->default_value		= "0";
		break;
	case PN_MODE:
		properties->name				= "Mode";
		properties->variable_address	= &mode;
		properties->datatype_extra		= "latch=0,delaylatch=1,onesample=2";
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_ENUM;
		properties->default_value		= "0";
		break;
	default:
		return false; // host will ask for plugs 0,1,2,3 etc. return false to signal when done
	};

	return true;
}

