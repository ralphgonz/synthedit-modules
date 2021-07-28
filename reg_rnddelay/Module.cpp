/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// Random Delay Copyright 2003 Ralph Gonzalez

#include "Module.h"
#include "SEMod_struct.h"
#include "SEPin.h"
#include <assert.h>
#include <stdlib.h>

// instead of refering to the pins by number, give them all a name
#define PN_GATE			0
#define PN_INPUT		1
#define PN_GAIN			2
#define PN_DELAY		3
#define PN_GAIN_WIDTH	4
#define PN_DELAY_WIDTH	5
#define PN_PULSE_WIDTH	6
#define PN_OUTPUT		7
#define PN_BUFFER		8
#define PN_FADE_IN		9
#define PN_FADE			10
#define PN_UNITIN		11

Module::Module(seaudioMasterCallback seaudioMaster) : SEModule_base(seaudioMaster)
,buffer(0)
{
	buffer = NULL;
}

Module::~Module()
{
}

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *gateP  = buffer_offset + gate_buffer;
    float *inputP  = buffer_offset + input_buffer;
    float *gainP = buffer_offset + gain_buffer;
    float *delayP  = buffer_offset + delay_buffer;
	float *gwidthP = buffer_offset + gain_width_buffer;
    float *dwidthP  = buffer_offset + delay_width_buffer;
	float *pwidthP = buffer_offset + pulse_width_buffer;
    float *outputP = buffer_offset + output_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		bool current_gate = (*gateP++ != 0.);
		float in=*inputP++;
		float g=*gainP++;
		float d=*delayP++;
		float gw=*gwidthP++;
		float dw=*dwidthP++;
		float pulse_width=*pwidthP++;
		float out=0.;

		if (unitIn==1)
			in = 1.f;

		if (!gate_open && current_gate)
		{
			gate_open = sampling = looping = true;
			gdelta = 1.;
			getWidths(d,dw,gw,&loop_size,&gdelta_next);
			sample_count = fade_in_count = loop_count = 0;
		}
		else if (gate_open && !current_gate)
			gate_open = false;

		if (sampling)
		{
			if (++fade_in_count < fade_in_size)
				in *= (fade_in_count/(float)fade_in_size);
			buffer[sample_count] = in;
			if (++sample_count >= buffer_size)
				sampling = false;
		}

		if (fading)
		{
			out += buffer[fade_pos + fade_count] *
				(fade_size-fade_count)/(float)fade_size * g * fade_gdelta;
			if (++fade_count >= fade_size)
				fading = false;
		}

		if (looping)
		{
			if (loop_count < (int)(loop_size*pulse_width+.5f))
				out += buffer[loop_count] * g * gdelta;
			if (++loop_count == (int)(loop_size*pulse_width+.5f))
			{
				fade_pos = loop_count;
				fade_count = 0;
				fading = true;
				fade_gdelta = gdelta;
			}
			if (loop_count >= loop_size)
			{
				loop_count = 0;
				gdelta = gdelta_next;
				getWidths(d,dw,gw,&loop_size,&gdelta_next);
			}
		}

		*outputP++ = out;
	}
}

void Module::getWidths(float d,float dw,float gw,int* dp,float* gp)
{
	float r=rand()/(float)RAND_MAX; // [0,1]
	int min=(int)(buffer_size * d * (1.-dw));
	int max=(int)(buffer_size * d * (1.+dw));
	if (min < fade_size)
		min = fade_size;
	if (min >= buffer_size - fade_size)
		min = buffer_size - fade_size - 1;
	if (max >= buffer_size - fade_size)
		max = buffer_size - fade_size - 1;

	*dp = (int)((max - min) * r + min);
	*gp = (float)(2.*gw*r + (1.-gw));
	if (*gp < 0.)
		*gp = 0.;
}

void Module::close()
{
	delete [] buffer;

	SEModule_base::close();
}

// perform any initialisation here
void Module::open()
{
	// this macro switches the active processing function
	// e.g you may have several versions of your code, optimised for a certain situation
	SET_PROCESS_FUNC(Module::sub_process);

	SEModule_base::open(); // call the base class

	max_buffer_size = 0;
	gate_open = sampling = looping = fading = false;
	CreateBuffer();

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
	properties->name = "Random Delay";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG Random Delay";

	// Info, may include Author, Web page whatever
	properties->about = "Random Delay v.1.0.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
July 2003 \n\
\n\
Licensing: free to use and distribute, if documentation and \n\
credits accompany. Contact author for distribution in Synthedit \n\
(eg .se1) or VST projects. \n\
\n\
Samples first MAX BUFFER ms of INPUT, following GATE signal. \n\
Then plays random-length portions of the buffer to give the effect \n\
of an echo with random delay times. \n\
\n\
A gain envelope may be applied to GAIN. A separate envelope may be \n\
applied to the DELAY input if desired, to vary the mean delay time.\n\
The amount of randomness in the actual delay depends on the RND DELAY \n\
value. If this is 0, then you have a conventional fixed-length delay. \n\
Likewise, the RND GAIN input lets you apply a degree of randomness to \n\
the gain. (The actual random value is the same used for the delay, so \n\
short echoes are quieter than long echoes.) \n\
\n\
TO avoid clicks, the sampler fades in over FADE IN ms (default=1 ms).\n\
The echo overlaps the next one and fades out over FADE OUT ms (default=10 ms).\n\
You can use this crossfade effect to create smoothly-varying textures.\n\
\n\
Use the PULSE WIDTH parameter to play the sample a percent of\n\
the delay time. Use UNIT IN to supply 1 for the input. In combination\n\
with the PULSE WIDTH parameter this produces a random tremolo.";
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
	case PN_INPUT:
		properties->name				= "Input";
		properties->variable_address	= &input_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_GAIN:
		properties->name				= "Gain";
		properties->variable_address	= &gain_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "1";
		break;
	case PN_DELAY:
		properties->name				= "Delay";
		properties->variable_address	= &delay_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= ".5";
		break;
	case PN_GAIN_WIDTH:
		properties->name				= "Rnd gain";
		properties->variable_address	= &gain_width_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_DELAY_WIDTH:
		properties->name				= "Rnd delay";
		properties->variable_address	= &delay_width_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_PULSE_WIDTH:
		properties->name				= "Pulse Width";
		properties->variable_address	= &pulse_width_buffer;
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
	case PN_BUFFER:
		properties->name				= "Max Buffer ms";
		properties->variable_address	= &buffer_ms;
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_FLOAT;
		properties->default_value		= "1000";
		break;
	case PN_FADE_IN:
		properties->name				= "Fade In ms";
		properties->variable_address	= &fade_in_ms;
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_FLOAT;
		properties->default_value		= "1";
		break;
	case PN_FADE:
		properties->name				= "Fade Out ms";
		properties->variable_address	= &fade_ms;
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_FLOAT;
		properties->default_value		= "10";
		break;
	case PN_UNITIN:
		properties->name				= "Unit In";
		properties->variable_address	= &unitIn;
		properties->datatype_extra		= "extIn=0,UnitIn=1";
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_ENUM;
		properties->default_value		= "0";
		break;
	default:
		return false; // host will ask for plugs 0,1,2,3 etc. return false to signal when done
	};

	return true;
}

// allocate some memory to hold the delayed samples
// TODO: call this when values change while running
void Module::CreateBuffer()
{
	buffer_size = (int) ( getSampleRate() * ((float)buffer_ms) / 1000.f );
	if( buffer_size < 1 )
		buffer_size = 1;

	if( buffer_size > getSampleRate() * 10 )	// limit to 10 s sample
		buffer_size = (int)getSampleRate() * 10;

	if (!buffer || buffer_size > max_buffer_size)
	{
		if (buffer)
			delete [] buffer;
		buffer = new FSAMPLE[buffer_size];
		max_buffer_size = buffer_size;
	}

	fade_in_size = (int) (getSampleRate() * ((float)fade_in_ms) / 1000.f);
	if (fade_in_size >= buffer_size)
		fade_in_size = buffer_size - 1;

	fade_size = (int) (getSampleRate() * ((float)fade_ms) / 1000.f);
	if (fade_size >= buffer_size)
		fade_size = buffer_size - 1;

	// clear buffer
	memset(buffer, 0, buffer_size * sizeof(FSAMPLE) ); // clear buffer
}

