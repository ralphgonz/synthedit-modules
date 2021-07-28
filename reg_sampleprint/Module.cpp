/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// PrintChart Copyright 2003 Ralph Gonzalez

#include "Module.h"
#include "SEMod_struct.h"
#include "SEPin.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>

// instead of refering to the pins by number, give them all a name
#define PN_INPUT	0
#define PN_INPUT2	1
#define PN_INPUT3	2
#define PN_INPUT4	3
#define PN_FNAME	4
#define PN_WIDTH	5

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
    float *in2P  = buffer_offset + input2_buffer;
    float *in3P  = buffer_offset + input3_buffer;
    float *in4P  = buffer_offset + input4_buffer;

	if (fp==NULL)
		fp = fopen(fname, "w");	// fname is set now

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float in=*inP++;
		float in2=*in2P++;
		float in3=*in3P++;
		float in4=*in4P++;

		secs += (float)(1./sRate);
		if (count < period)
			++count;
		else
		{
			static char buf[128];
			static char buf2[32];
			count = 0;
			int mins=(int)(secs/60);
			int ssecs=(int)(secs-mins*60);
			int msec=(int)((secs-mins*60-ssecs)*1000);
			sprintf(buf,"%d:%02d:%04d", mins,ssecs,msec);
			if (getPin(PN_INPUT)->IsConnected())
			{
				sprintf(buf2,"\t%f",in);
				strcat(buf,buf2);
			}
			if (getPin(PN_INPUT2)->IsConnected())
			{
				sprintf(buf2,"\t%f",in2);
				strcat(buf,buf2);
			}
			if (getPin(PN_INPUT3)->IsConnected())
			{
				sprintf(buf2,"\t%f",in3);
				strcat(buf,buf2);
			}
			if (getPin(PN_INPUT4)->IsConnected())
			{
				sprintf(buf2,"\t%f",in4);
				strcat(buf,buf2);
			}
			if (fp!=NULL)
				fprintf(fp,"%s\n", buf);
		}
	}
}

void Module::close()
{
	if (fp!=NULL)
		fclose(fp);
	SEModule_base::close();
}

// perform any initialisation here
void Module::open()
{
	// this macro switches the active processing function
	// e.g you may have several versions of your code, optimised for a certain situation
	SET_PROCESS_FUNC(Module::sub_process);

	SEModule_base::open(); // call the base class

	sRate = getSampleRate();
	period = (int) ( sRate * ((float)width) / 1000.f );
	if( period < 1 )
		period = 1;
	count=0;
	secs = 0;
	fp = NULL;
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
	properties->name = "PrintChart";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG Print Chart";

	// Info, may include Author, Web page whatever
	properties->about = "Print Chart v.1.0 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
July 2003 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
This module is mainly for debugging. Prints value of up to 4 inputs \n\
to FILE (default 'chart.txt' in Synthedit home directory) every \n\
PERIOD ms (default=1000). Also prints time stamp. \n\
\n\
Beware of using periods smaller than 10 ms, as this increases disk \n\
and cpu activity significantly.\
";
	return true;
}

// describe the pins (plugs)
bool Module::getPinProperties (long index, SEPinProperties* properties)
{
	switch( index )
	{
	case PN_INPUT:
		properties->name				= "In";
		properties->variable_address	= &input_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_INPUT2:
		properties->name				= "In2";
		properties->variable_address	= &input2_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_INPUT3:
		properties->name				= "In3";
		properties->variable_address	= &input3_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_INPUT4:
		properties->name				= "In4";
		properties->variable_address	= &input4_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_FNAME:
		properties->name				= "File";
		properties->variable_address	= &fname;
		properties->direction			= DR_IN;
		properties->datatype			= DT_TEXT;
		properties->default_value		= "chart.txt";
		break;
	case PN_WIDTH:
		properties->name				= "Period ms";
		properties->variable_address	= &width;
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_FLOAT;
		properties->default_value		= "1000";
		break;
	default:
		return false; // host will ask for plugs 0,1,2,3 etc. return false to signal when done
	};

	return true;
}
