/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// FMLFO Copyright 2004 Ralph Gonzalez

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

#define MAXSEC			30	// max seconds for LFO

// instead of refering to the pins by number, give them all a name
#define PN_IN			0
#define PN_DEPTH		1
#define PN_OFFSET		2
#define PN_FREQ			3
#define PN_PHASE		4
#define PN_WAVE			5
#define PN_OUT			6
#define PN_FSCALE		7
#define PN_DSCALE		8

// freq scale:
#define FSCALE_HZ		0
#define FSCALE_KHZ		1

// depth scale:
#define DSCALE_ABS		0
#define DSCALE_REL		1

// waves:
#define SINEWAVE		0
#define TRIWAVE			1
#define SQUAREWAVE		2

Module::Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1) : SEModule_base(seaudioMaster, p_resvd1)
{
	mySin = NULL;
	maBuf = NULL;
}

Module::~Module()
{
}

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *inP  = buffer_offset + in_buffer;
    float *depthP  = buffer_offset + depth_buffer;
    float *offsetP  = buffer_offset + offset_buffer;
    float *freqP  = buffer_offset + freq_buffer;
    float *phaseP  = buffer_offset + phase_buffer;
    float *outP = buffer_offset + out_buffer;
	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float in=*inP++;
		float offset=*offsetP++;
		float depth=(*depthP++) * (dscale==DSCALE_ABS ? 1.f : offset);
		float freq=(*freqP++) * (fscale==FSCALE_HZ ? 10.f : 10000.f);
		float phase=(*phaseP++) / 36.f;

		++pos;
		if (freq <= 1./MAXSEC || depth == 0.f)
		{
			*outP++ = in * offset;
			pos = prevPeriod = -1;
		}
		else
		{
			int period=(int)(SAMPLEFREQ / freq + .5f);
			if (prevPeriod > 0 && prevPeriod != period)
				pos = (int)(pos/((float)prevPeriod)*((float)period) + .5f);
			prevPeriod = period;
			pos %= period;
			int pos2=pos + (int)(phase * period + .5f);
			pos2 %= period;
			*outP++ = in * (offset + depth * waveform(wave,pos2,period));
		}
#ifdef DDEBUG
//fprintf(fp,"%d\n",
//	pos);
#endif
	}
}

void Module::close()
{
	delete [] mySin;
	delete [] maBuf;
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
fp = fopen("xxx.txt","w");
if (fp==NULL)
  exit(1);
#endif

	SAMPLEFREQ = (int)getSampleRate();
	MAXSIN = SAMPLEFREQ * MAXSEC;
	mySin = new float[MAXSIN];

	for (int i=0 ; i<MAXSIN ; ++i)
		mySin[i] = (float)sin((double)(i/((float)MAXSIN)*3.1415926*2.));

	maSum = 0.f;
	maCount = 0;
	maLengthPrev = 0;
	// This is about 1.2 ms, 50 samples at 44.1k:
	MAMAX = 1 + (maLength = (int)(SAMPLEFREQ/882.f));
	maBuf = new float[MAMAX];
	memset(maBuf, 0, MAMAX * sizeof(float) );

	pos = prevPeriod = -1;

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
	properties->name = "FMLFO";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG FMLFO";

	// Info, may include Author, Web page whatever
	properties->about = "FMLFO v.1.0.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
May 2004 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
See also SynthEdit's Oscillator module \n\
\n\
Generates LFO using choice of waveforms. FREQ input may \n\
itself be modulated by (eg) another FMLFO without zippering \n\
problems. Likewise for PHASE input. You can make a FM or PM \n\
synth by applying frequency (eg from MIDItoCV) to an FMLFO \n\
and a multiple of this frequency to another FMLFO which \n\
itself modulates the FREQ or PHASE input of the first. \n\
OK, it's not really a L(ow) F(requency) O(scillator) then... \n\
\n\
For a tremolo effect, set the OFFSET to 1, DEPTH to 1, \n\
FREQ in the 0.1 to 10 Hz range, and apply the FMLFO to \n\
the gain of an audio signal, OR, simply apply the audio \n\
signal to the FMLFO's optional IN pin. \n\
\n\
For a stereo chorus/flanger, use two FMLFO filters on two \n\
delay filters, with OFFSET=delay, DEPTH=delay variation. Adjust \n\
the PHASE of the FMLFO's differently so each FMLFO is \n\
at a different point in the LFO and mix the delay outputs \n\
in with the signal to obtain L and R stereo outputs. \n\
\n\
If DEPTH SCALE=relative then a value of 1 equals the offset \n\
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
		properties->default_value		= "10";
		properties->flags				= IO_LINEAR_INPUT;
		break;
	case PN_DEPTH:
		properties->name				= "Depth";
		properties->variable_address	= &depth_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "5";
		properties->datatype_extra		= "10,0,10,0";
		break;
	case PN_OFFSET:
		properties->name				= "Offset";
		properties->variable_address	= &offset_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		properties->datatype_extra		= "10,0,10,0";
		break;
	case PN_FREQ:
		properties->name				= "Freq";
		properties->variable_address	= &freq_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "1";
		properties->datatype_extra		= "20,.0001,20,.0001";
		break;
	case PN_PHASE:
		properties->name				= "Phase";
		properties->variable_address	= &phase_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		properties->datatype_extra		= "360,0,360,0";
		break;
	case PN_WAVE:
		properties->name				= "Wave";
		properties->variable_address	= &wave;
		properties->datatype_extra		= "sine=0,triangle=1,square=2";
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
	case PN_FSCALE:
		properties->name				= "Freq scale";
		properties->variable_address	= &fscale;
		properties->datatype_extra		= "Hz=0,kHz=1";
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_ENUM;
		properties->default_value		= "0";
		break;
	case PN_DSCALE:
		properties->name				= "Depth scale";
		properties->variable_address	= &dscale;
		properties->datatype_extra		= "Absolute=0,Relative=1";
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_ENUM;
		properties->default_value		= "0";
		break;
	default:
		return false; // host will ask for plugs 0,1,2,3 etc. return false to signal when done
	};

	return true;
}

float Module::waveform(short wave, int i, int period)
{
	float ret=0.f;

	switch (wave)
	{
	case SQUAREWAVE:
		if (i/(float)period <.5f)
			ret = 1.f;
		else
			ret = -1.f;
		// cheap way to round edges to avoid static noise:
		ret = movAvg(ret, maLength, maLengthPrev, maCount, maBuf, maSum);
		break;

	case SINEWAVE:
		ret = mySin[(int)(((float)MAXSIN)*i/(float)period)];
		break;

	case TRIWAVE:
		ret = i * 4.f / (float)period;
		if (ret < 1.f)
			;
		else if (ret < 3.f)
			ret = 2.f-ret;
		else
			ret = ret-4.f;
		break;

	default:
		break;
	}

	return ret;
}

#define MOD(i,n) ((i)>=0 ? (i) : (i)+(n))

// Highly efficient "boxcar" digital filter, simply
// the moving average of last n samples. This has a lot
// of stopband variation, but ok in this application where
// the output is a comb filter anyway.
// Ring-buffer contains history.
float Module::movAvg(float x, int n, int& maLengthPrev, int& maCount, float maBuf[], float& maSum)
{

	maBuf[maCount] = x;

	if (n<=1)
		maSum = x;
	else if (n != maLengthPrev)
	{
		maSum = 0.;
		for (int i=0 ; i<n ; ++i)
			maSum += maBuf[MOD(maCount-i,MAMAX)];
	}
	else
	{
		maSum += maBuf[maCount];
		maSum -= maBuf[MOD(maCount-n,MAMAX)];
		maSum *= .9999f;	// for stability
	}

#ifdef DDEBUG
	fprintf(fp,"%f %f\n",
		maBuf[maCount],maBuf[MOD(maCount-n,MAMAX)]);
#endif
	++maCount;
	maCount %= MAMAX;
	maLengthPrev = n;

	return (n<=1 ? maSum : maSum/n);
}
