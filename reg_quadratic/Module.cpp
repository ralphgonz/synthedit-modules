/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// Quadratic Copyright 2003 Ralph Gonzalez

#include "Module.h"
#include "SEMod_struct.h"
#include "SEPin.h"
#include <assert.h>
#include <stdlib.h>

// instead of refering to the pins by number, give them all a name
#define PN_X		0
#define PN_Y		1
#define PN_Z		2
#define PN_OUTPUT	3
#define PN_A		4
#define PN_B		5
#define PN_C		6

Module::Module(seaudioMasterCallback seaudioMaster) : SEModule_base(seaudioMaster)
{
}

Module::~Module()
{
}

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *xP  = buffer_offset + x_buffer;
    float *yP  = buffer_offset + y_buffer;
    float *zP = buffer_offset + z_buffer;
    float *outputP = buffer_offset + output_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		*outputP++ = a + b*(*xP++) + c*(*yP++)*(*zP++);
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
	properties->name = "Quadratic";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG Quadratic";

	// Info, may include Author, Web page whatever
	properties->about = "Quadratic v.1.0.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
July 2003 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
A multi-variable polynomial expression. Much less flexible than \n\
Unkargherth's U-MathEv, but efficient and useful for simple sums, \n\
differences, and products of inputs. \n\
\n\
Inputs are X (default 0), Y (default 0), and Z (default 1). Output is: \n\
\n\
    A + B * X + C * Y * Z \n\
\n\
The coefficients are parameters defaulting to A=0,B=1,C=1. \n\
\n\
For example, to subtract the product of two inputs from the number 1, \n\
set A=1, B=0, C=-1, and attach the inputs to Y and Z (leave X \n\
unconnected). To square an input, use the same input on Y and Z.\
";
	return true;
}

// describe the pins (plugs)
bool Module::getPinProperties (long index, SEPinProperties* properties)
{
	switch( index )
	{
	case PN_X:
		properties->name				= "X";
		properties->variable_address	= &x_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_Y:
		properties->name				= "Y";
		properties->variable_address	= &y_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_Z:
		properties->name				= "Z";
		properties->variable_address	= &z_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_OUTPUT:
		properties->name				= "Output";
		properties->variable_address	= &output_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case PN_A:
		properties->name				= "A";
		properties->variable_address	= &a;
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_FLOAT;
		properties->default_value		= "0";
		break;
	case PN_B:
		properties->name				= "B";
		properties->variable_address	= &b;
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_FLOAT;
		properties->default_value		= "1";
		break;
	case PN_C:
		properties->name				= "C";
		properties->variable_address	= &c;
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_FLOAT;
		properties->default_value		= "1";
		break;
	default:
		return false; // host will ask for plugs 0,1,2,3 etc. return false to signal when done
	};

	return true;
}

