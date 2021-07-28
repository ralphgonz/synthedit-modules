/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// NewWave Copyright 2004 Ralph Gonzalez

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
#define PN_MININ		1
#define PN_TRANS		2
#define PN_WAVE			3
#define PN_PMDEPTH		4
#define PN_PMRATIO		5
#define PN_OUT			6
#define PN_FREQ			7
#define PN_AMP			8
#define PN_HYSTER		9

// states:
#define SILENT	0
#define PREP	1
#define RAMPUP	2
#define ACTIVE	3
#define RAMPDN	4

// waves:
#define SQUAREWAVE	0
#define SINEWAVE	1
#define TRIWAVE		2

// mod function -- C's % operator doesn't do neg like perl does
// This version assumes i >= -n
#define MOD(i,n) ((i)>=0 ? (i)%(n) : (i)+(n))

Module::Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1) : SEModule_base(seaudioMaster, p_resvd1)
{
	mySin = NULL;
}

Module::~Module()
{
}

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *inP  = buffer_offset + in_buffer;
    float *mininP  = buffer_offset + minin_buffer;
    float *transP  = buffer_offset + trans_buffer;
	float *pmDepthP = buffer_offset + pmDepth_buffer;
	float *pmRatioP = buffer_offset + pmRatio_buffer;
    float *outP = buffer_offset + out_buffer;
    float *outFP = buffer_offset + outF_buffer;
    float *outAP = buffer_offset + outA_buffer;
	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float in=*inP++;
		float minin=*mininP++;
		float rawTrans=*transP++;
		float pmDepth=*pmDepthP++;
		float pmRatio=(*pmRatioP++) * 10.f;
		float out=0.f;
		if (rawTrans!=prevRawTrans)
		{
			trans = (float)pow(2.f,rawTrans*10.);
			prevRawTrans = rawTrans;
		}

		++count;
		++newCount;
		++pmCount;
		if (fabs(in) < minin * (state!=SILENT ? hyster : 1.0f))	// in noise floor
		{
		}
		else if (in < 0.f && !wentNeg)	// just going negative
		{
			wentNeg = true;
			period = newCount;
			if (state == SILENT)
				state = PREP;	// wait to get a valid period
			else if (state == PREP && period > 0)
			{
				state = RAMPUP;	// start first cycle, fade in
				count = 0;
				prevPos = 100000;
				transPeriod = (int)(period / trans + .5f);
				pmCount = 0;
				pmPeriod = (int)(transPeriod / pmRatio + .5f);
			}
			pendingPeriod = (int)(period / trans + .5f);
			pendingAmp = newAmp;
			// reset counts:
			newCount = 0;
			newAmp = 0.f;
		}
		else if (in > 0.f)
		{
			wentNeg = false;
			if (in > newAmp)	// maybe found max
				newAmp = in;
		}


		float scale=0.f;
		int pos=0;
		if (transPeriod>0 && (pos=(count%transPeriod))==0)	// starting new cycle, ok to change amp:
		{
			count = 0;
			if (state!=RAMPDN)
			{
				prevAmp = amp;
				amp = pendingAmp;
				transPeriod = pendingPeriod;
				freq = SAMPLEFREQ / (float)transPeriod / 1000.f; // V/khz
				pmCount %= pmPeriod;
				int newPMPeriod = (int)(transPeriod / pmRatio + .5f);
				pmCount = (int)(pmCount / ((float)pmPeriod) * ((float)newPMPeriod));
				pmPeriod = newPMPeriod;
			}
		}
		prevPos = pos;

		switch (state)
		{
		case SILENT:
		case PREP:
			scale = 0.f;
			freq = .01f;
			break;

		case RAMPUP:
			scale = amp * pos / ((float)transPeriod);
			if (pos == transPeriod-1)
				state = ACTIVE;
			// sanity check so we don't have to wait for long bogus period:
			if (pendingPeriod < (int)(transPeriod/1.5f))
			{
				state = RAMPDN;
				prevAmp = scale;
				downPeriod = downPos = pendingPeriod;
			}
			break;

		case ACTIVE:
			scale = prevAmp + (amp-prevAmp) * pos / ((float)transPeriod);
			if (newCount > (int)(1.5f*period))	// missed zero-crossing, fade out
			{
				state = RAMPDN;
				prevAmp = scale;
				downPeriod = downPos = transPeriod;
			}
			break;

		case RAMPDN:
			scale = prevAmp * downPos/((float)downPeriod);
			if (--downPos == 0)
			{
				state = SILENT;
				count = newCount = period = transPeriod = pendingPeriod = -1;
				amp = prevAmp = pendingAmp = newAmp = 0.;
				prevPos = 100000;
				freq = .01f;
				pmCount = pmPeriod = 0;
			}
			break;
		}

#define PM_FUDGE 10		// assume we have 10x headroom in precomputed SINE period
		int pos2=pos;
		int transPeriod2=transPeriod;
		if (pmDepth > 0. && pmPeriod > 0)
		{
			// apply phase modulation to pos2. Need FUDGE so
			// effectively-longer periods appear as continuous waveforms
			pos2 *= PM_FUDGE;
			transPeriod2 *= PM_FUDGE;
			pmDepth *= transPeriod2;	// should do less frequently
			pos2 += (int)(pmDepth * waveform(SINEWAVE, pmCount%pmPeriod, pmPeriod) + .5f);
			pos2 = MOD(pos2, transPeriod2);
		}

		// find output:
		if (scale > 0.f && freq > .02f && freq < 20.f)
			out = scale * waveform(wave, pos2, transPeriod2);

		*outP++ = out;
		*outFP++ = freq;
		*outAP++ = scale;

#ifdef DDEBUG
fprintf(fp,"%d %d %d %d %.3f\n",
	pmCount,pmPeriod,pos2,transPeriod,pmDepth);
#endif
	}
}

void Module::close()
{
	delete [] mySin;
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

	SAMPLEFREQ = (int)getSampleRate();
	MAXSIN = SAMPLEFREQ;
	mySin = new float[MAXSIN];

	for (int i=0 ; i<MAXSIN ; ++i)
		mySin[i] = (float)sin((double)(i/((float)MAXSIN)*3.1415926*2.));

	period = count = newCount = transPeriod = pendingPeriod = -1;
	freq = .01f;
	pmCount = pmPeriod = 0;
	state = SILENT;
	wentNeg = false;
	amp = newAmp = pendingAmp = 0.f;
	prevRawTrans = 0.f;
	trans = 1.f;
	prevPos = 100000;

	// this macro switches the active processing function
	// e.g you may have several versions of your code, optimised for a certain situation
	SET_PROCESS_FUNC(Module::sub_process);

	SEModule_base::open(); // call the base class

	// let 'downstream' modules know audio data is coming
	getPin(PN_OUT)->TransmitStatusChange( SampleClock(), ST_RUN );
	getPin(PN_FREQ)->TransmitStatusChange( SampleClock(), ST_RUN );
	getPin(PN_AMP)->TransmitStatusChange( SampleClock(), ST_RUN );
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
	properties->name = "NewWave";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG NewWave";

	// Info, may include Author, Web page whatever
	properties->about = "NewWave v.1.1.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
May 2004 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
Substitutes each cycle of the (monophonic) input waveform \n\
with a synthetic waveform. You should set the minimum input level, \n\
and 'Off ratio', which reduces this threshold to find when \n\
the signal stops. \n\
\n\
Waveform can be phase modulated (similar to FM synthesis) \n\
by setting the PM Ratio and using a non-zero PM Depth. \n\
\n\
Use external low-pass filter and/or compression on input signal to \n\
improve effectiveness. Be sure to use a 'dry' input, without \n\
real or artificial echoes. \n\
\n\
This method is more responsive than the usual frequency- \n\
discovery approach which must be followed by a separate \n\
oscillator/envelope. \n\
\n\
A secondary (V/kHz scale) Freq output can be used to control \n\
a following Moog-type lowpass filter, for example. The amplitude \n\
output can also optionally control an oscillator or filter. \n\
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
	case PN_MININ:
		properties->name				= "MinIn";
		properties->variable_address	= &minin_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= ".1";
		break;
	case PN_TRANS:
		properties->name				= "Trans";
		properties->variable_address	= &trans_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		properties->datatype_extra		= "4,-4,4,-4";
		break;
	case PN_WAVE:
		properties->name				= "Wave";
		properties->variable_address	= &wave;
		properties->datatype_extra		= "square=0,sine=1,triangle=2";
		properties->direction			= DR_IN;
		properties->datatype			= DT_ENUM;
		properties->default_value		= "0";
		break;
	case PN_PMDEPTH:
		properties->name				= "PM Depth";
		properties->variable_address	= &pmDepth_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		break;
	case PN_PMRATIO:
		properties->name				= "PM Ratio";
		properties->variable_address	= &pmRatio_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "2";
		properties->datatype_extra		= "10,.1,10,.1";
		break;
	case PN_OUT:
		properties->name				= "Out";
		properties->variable_address	= &out_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case PN_FREQ:
		properties->name				= "Freq";
		properties->variable_address	= &outF_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case PN_AMP:
		properties->name				= "Amp";
		properties->variable_address	= &outA_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case PN_HYSTER:
		properties->name				= "Off ratio";
		properties->variable_address	= &hyster;
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_FLOAT;
		properties->default_value		= ".5";
		break;
	default:
		return false; // host will ask for plugs 0,1,2,3 etc. return false to signal when done
	};

	return true;
}

float Module::waveform(short wave, int i, int period)
{
	float ret;

	switch (wave)
	{
	case SQUAREWAVE:
		if (i/(float)period <.5f)
			return 1.f;
		else
			return -1.f;
		break;

	case SINEWAVE:
		return mySin[(int)(((float)MAXSIN)*i/(float)period)];
		break;

	case TRIWAVE:
		ret = i * 4.f / (float)period;
		if (ret < 1.f)
			return ret;
		else if (ret < 3.f)
			return 2.f-ret;
		else
			return ret-4.f;
		break;

	default:
		return 0.f;
		break;
	}
}

