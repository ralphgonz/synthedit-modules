/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// BadHead Copyright 2004 Ralph Gonzalez
// v.1.0.3 04/05/06 Added sleep mode support
// v.1.1 09/05/07 Added saturation support

#include "Module.h"
#include "SEMod_struct.h"
#include "SEPin.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>
//#include <stdio.h>

// instead of refering to the pins by number, give them all a name
#define PN_IN		0
#define PN_GAIN		1
#define PN_BIAS		2
#define PN_CLIP		3
#define PN_SPEED	4
#define PN_CAP		5
#define PN_SHAPE	6
#define PN_OUT		7

#define CLIP_SHAPE	0
#define KINK_SHAPE	1
#define SATURATE_SHAPE	2

Module::Module(seaudioMasterCallback seaudioMaster) : SEModule_base(seaudioMaster)
{
}

Module::~Module()
{
}

# define SQR(x) ((x)*(x))
# define SPEEDK 2.f
# define CAPMAX 10.f
# define CAPSCALE 1000.f
# define CLIPMAX 10.f
# define CLIPSCALE (1.f/512.f)	// depends on CLIPMAX

//FILE* fp;

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *inP  = buffer_offset + in_buffer;
    float *gainP  = buffer_offset + gain_buffer;
    float *biasP = buffer_offset + bias_buffer;
    float *clipP = buffer_offset + clip_buffer;
    float *speedP = buffer_offset + speed_buffer;
    float *capP = buffer_offset + cap_buffer;
    float *outP = buffer_offset + out_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float clip=*clipP++;
		clip *= clip;	// scale by squaring
		bool clipChanged=false;
//		fprintf(fp,"%f %f\n",clip,prevClip);
		if (clip != prevClip)
		{
			// logarithmic value [0,1] -> [2,.00195] //[.0039,1]:
			clip2 = (float)(pow(2.,(1.-clip)*CLIPMAX) * CLIPSCALE);
			xAdj = 1.f; // offset x by 1
//			xAdj = 1.f - SQR(clip2)/4.f; // solve root of hyperbola below
			prevClip = clip;
			clipChanged = true;	// have to reset bias too
		}
		float speed=*speedP++;
		float cap=*capP++;
		if (cap != prevCap)
		{
			// logarithmic value [0,1] -> [1000,1M]
			cap2 = (float)(pow(2.,cap*CAPMAX) * CAPSCALE);
			prevCap = cap;
		}
		float bias=*biasP++;
		float gain=*gainP++;
		if (gain != prevGain)
		{
			// log value [0,1] -> [1/8,128]
			gain2 = (float)pow(2.,(gain-.3)*10.);
			prevGain = gain;
		}
		float x0=bias - 1.f;
		// remove DC offset and splice hyperbolae together at x=0:
		if (x0 != prevX0 || clipChanged || shape != prevShape)
		{
			float tmpX=x0 + xAdj;
			if (shape != KINK_SHAPE)
				dcOffset = (float)((clip2*sqrt(1.+SQR(tmpX/clip2)) + tmpX)*.5 - 1.);
			else
				dcOffset = (float)((tmpX - clip2*sqrt(1.+SQR(tmpX/clip2)))*.5);
			tmpX=xAdj;
			if (shape != KINK_SHAPE)
				dcOffsetNeg = (float)((clip2*sqrt(1.+SQR(tmpX/clip2)) + tmpX)*.5 - 1.);
			else
				dcOffsetNeg = (float)((tmpX - clip2*sqrt(1.+SQR(tmpX/clip2)))*.5);
			dcOffset -= dcOffsetNeg;
			tmpX=-xAdj;
			if (shape != KINK_SHAPE)
				dcOffsetPos = (float)(1. - .5*(clip2*sqrt(1.+SQR(tmpX/clip2)) - tmpX));
			else
				dcOffsetPos = (float)((tmpX + clip2*sqrt(1.+SQR(tmpX/clip2)))*.5);
			prevX0 = x0;
			prevShape = shape;
		}

		float x=*inP++;
		if (shape == KINK_SHAPE)
			x *= 4.f;	// scale temporarily to move kink point
		else if (shape == SATURATE_SHAPE)
		    	x *= (bias < 0.1f ? 0.1f : bias); // scale temporarily to avoid clipping
		x = x * gain2 / charge + x0;
		float out;
		// Below is formula of hyperbola y=0.5*(k*sqrt(1+x^2/k^2)+x) - 1
		// This is rotated 45 degrees by x term and offset vertically by - 1
		// For shape==KINK_SHAPE, it is not offset vertically, and neg/pos formulas are swapped
		if (x<=0.)
		{
			x += xAdj;
			if (shape != KINK_SHAPE)
				out = (float)((clip2*sqrt(1.+SQR(x/clip2)) + x)*.5 - 1.);	// hyperbola
			else
				out = (float)((x - clip2*sqrt(1.+SQR(x/clip2)))*.5);
			out -= dcOffset + dcOffsetNeg;
		}
		else
		{
			x -= xAdj;
			if (shape != KINK_SHAPE)
				out = (float)(1. - .5*(clip2*sqrt(1.+SQR(x/clip2)) - x)); // hyperbola
			else
				out = (float)((x + clip2*sqrt(1.+SQR(x/clip2)))*.5);
			out -= dcOffset + dcOffsetPos;
		}

		// compensate charge
		out *= charge;
		if (shape != SATURATE_SHAPE)
		    out /= (2.f - bias);    // to guarantee output remains < 1 for large inputs
		// also reduce level according to difference of dcoffsets:
		out /= (float)(1.f + .5f*fabs(dcOffsetPos-dcOffsetNeg));
		if (shape == KINK_SHAPE)
			out /= 4.f;	// undo temporary scaling
		else if (shape == SATURATE_SHAPE)
		    	out /= (bias < 0.1f ? 0.1f : bias); // undo temporary scaling
		*outP++ = out;
//		fprintf(fp,"%f %f\n", out, charge);

		// effect of regulation:
		if (cap < 1.)
		{
			float delta=(float)((speed*SPEEDK - fabs(out))/(cap2));
			float newCharge = charge + delta;
			if (newCharge > 1.)
				charge = .5f*(charge + 1.f);	// approach 1 gently
			else if (newCharge < .01)
				charge = .01f;
			else
				charge = newCharge;
		}
	}
}

void Module::sub_process_static(long buffer_offset, long sampleFrames )
{
	sub_process(buffer_offset, sampleFrames);
	static_count = static_count - sampleFrames;
	if( static_count <= 0 && !AreEventsPending() )
	{
		CallHost(seaudioMasterSleepMode);
	}
}

void Module::close()
{
	SEModule_base::close();
//	fclose(fp);
}

void Module::initVals()
{
	prevShape = CLIP_SHAPE;
	prevX0 = 0.f;
	dcOffset = dcOffsetPos = dcOffsetNeg = 0.f;
	charge = 1.f;
	prevClip = 1.e10;
	prevCap = 1.e10;
	prevGain = 1.e10;
}

// perform any initialisation here
void Module::open()
{
	initVals();

//	fp = fopen("xxx","w");

	// this macro switches the active processing function
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
	// query the 'state of the input plugs...
	//     ST_RUN     = Normal Streaming Audio        (e.g. from an oscillator)
	//     ST_STATIC  = Fixed, unchanging input value (e.g. a slider at rest)
	state_type in_stat = getPin(PN_IN)->getStatus();

	// we need to pass on the state of this module's output signal
	// it depends on the inputs...
	state_type out_stat = in_stat;

	// if user has set input to zero, we can tell downstream modules that the audio has stopped
	if( in_stat < ST_RUN && getPin(PN_IN)->getValue() == 0.f )
		out_stat = ST_STATIC;

	// 'transmit' this modules new output status to next module 'downstream'
	getPin(PN_OUT)->TransmitStatusChange( SampleClock(), out_stat);

	// setup up 'sleep mode' or not
	if( out_stat < ST_RUN )
	{
		static_count = getBlockSize();
		SET_PROCESS_FUNC(Module::sub_process_static);
	}
	else
	{
//		initVals();
		SET_PROCESS_FUNC(Module::sub_process);
	}
}

// describe the module
bool Module::getModuleProperties (SEModuleProperties* properties)
{
	// describe the plugin, this is the name the end-user will see.
	properties->name = "BadHead";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG BadHead";

	// Info, may include Author, Web page whatever
	properties->about = "BadHead v.1.1 \n\
Ralph Gonzalez ralphgonz@gmail.com \n\
May 2004 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
Yet another tube amp simulation. This one uses twin hyperbola \n\
functions to give adjustable tube saturation: how 'hard' the \n\
CLIP effect is.\n\
\n\
In addition to the signal input, you can connect a slider or \n\
modulation signal to the BIAS (default 10) and/or GAIN. \n\
The maximum BIAS gives symmetric clipping. Setting BIAS to 0 gives \n\
a rectified output, where only the upper half of the waveform passes. \n\
You can even set this value negative, beyond the rectification point. \n\
\n\
Another set of inputs, SPEED (default 10) and CAPACITY \n\
(default 10) let you simulate poor power supply regulation: \n\
auto-modulation of the distortion characteristics caused by \n\
loud output passages. Lowering the CAPACITY of the power supply \n\
makes this effect more pronounced. Lowering the SPEED (series \n\
resistance) of the power supply makes it recover more slowly from \n\
this condition. \n\
\n\
The 'Saturation' SHAPE is similar to the default 'Clip' SHAPE except \n\
that as you reduce the BIAS setting the gain is compensated to avoid clipping. \n\
With low BIAS settings this provides single-ended tube saturation rather \n\
than rectification. Note that in 'Saturation' mode the amp can pass a \n\
signal > 0dB, whereas 'Clip' mode guarantees the output never exceeds 0dB. \n\
\n\
Finally, if you use the 'Kink' instead of 'Clip/Saturate' options, then \n\
rather than clipping the input signal you get a central 'dead zone', \n\
similar to the crossover distortion produced by an underpowered \n\
tube push-pull output stage. \n\
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
	case PN_GAIN:
		properties->name				= "Gain";
		properties->variable_address	= &gain_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "3";
		break;
	case PN_BIAS:
		properties->name				= "Bias";
		properties->variable_address	= &bias_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_CLIP:
		properties->name				= "Clip";
		properties->variable_address	= &clip_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "5";
		break;
	case PN_SPEED:
		properties->name				= "Speed";
		properties->variable_address	= &speed_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_CAP:
		properties->name				= "Capacity";
		properties->variable_address	= &cap_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_SHAPE:
		properties->name				= "Shape";
		properties->variable_address	= &shape;
		properties->datatype_extra		= "Clip=0,Kink=1,Saturate=2";
		properties->direction			= DR_IN;
		properties->datatype			= DT_ENUM;
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

