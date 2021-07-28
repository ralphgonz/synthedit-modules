/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// RndAuto Copyright 2005 Ralph Gonzalez

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
#define PN_MINRANGE		0
#define PN_MAXRANGE		1
#define PN_FREQ			2
#define PN_RISE			3
#define PN_OUT			4

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
	float *minRangeP = buffer_offset + minRange_buffer;
	float *maxRangeP = buffer_offset + maxRange_buffer;
	float *freqP = buffer_offset + freq_buffer;
	float *riseP = buffer_offset + rise_buffer;
    float *outP = buffer_offset + out_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float minRange=(*minRangeP++);
		float maxRange=(*maxRangeP++);
		float freq=(*freqP++) * 10.f;
		float rise=(*riseP++) / 10.f;

		if (count==cycleLen)
		{
			if (cycleLen == 0)
				prevRnd = (maxRange + minRange) / 2.f;
			else
				prevRnd = rnd;
			// recompute cycleLen:
			count = 0;
			cycleLen = ((int)(SAMPPERSEC / freq));
			riseLen = (int)(rise * cycleLen + .5f);
			rnd = rand()/(float)RAND_MAX;
			rnd = rnd * (maxRange-minRange) + minRange;
		}
		if (count >= riseLen)
			*outP++ = rnd;
		else
			*outP++ = (rnd - prevRnd) * count / ((float)riseLen) + prevRnd;
		++count;
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

	count = 0;
	cycleLen = 0;

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
	properties->name = "RndAuto";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG RndAuto";

	// Info, may include Author, Web page whatever
	properties->about = "RndAuto v.1.0.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
Jan 2005 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
Random Automation module. Set minimum and maximum of output, \n\
and frequency of new random values. Also set fraction of this \n\
time changing between values. If you want the frequency to be \n\
random, use another RndAuto module to control this value. \n\
";
	return true;
}

// describe the pins (plugs)
bool Module::getPinProperties (long index, SEPinProperties* properties)
{
	switch( index )
	{
	case PN_MINRANGE:
		properties->name				= "Min";
		properties->variable_address	= &minRange_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		properties->datatype_extra		= "10,0,10,0";
		break;
	case PN_MAXRANGE:
		properties->name				= "Max";
		properties->variable_address	= &maxRange_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		properties->datatype_extra		= "10,0,10,0";
		break;
	case PN_FREQ:
		properties->name				= "Freq";
		properties->variable_address	= &freq_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "1";
		properties->datatype_extra		= "10,0.1,10,0.1";
		break;
	case PN_RISE:
		properties->name				= "Rise";
		properties->variable_address	= &rise_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "1";
		properties->datatype_extra		= "100,0,100,0";
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

