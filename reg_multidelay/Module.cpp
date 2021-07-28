/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// MultiDelay Copyright 2004 Ralph Gonzalez

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

#define INCHPERSEC 12320	// based on 700MPH
#define IPS "6160"			// box length at most 1/2 max

// mode values:
#define MULTITAP	0
#define INDEP		1

// scale values:
#define INCHES		0
#define MSEC		1

// instead of refering to the pins by number, give them all a name
#define PN_IN			0
#define PN_L1			1
#define PN_L2			2
#define PN_L3			3
#define PN_MIX1			4
#define PN_MIX2			5
#define PN_MIX3			6
#define PN_FEEDBACK		7
#define PN_MALENGTH		8
#define PN_OVERSAMPLE	9
#define PN_OUT			10
#define PN_POLARITY		11
#define PN_MODE			12
#define PN_SCALE		13
#define PN_MAXSECS		14

// handy function to fix denormals.
static const float anti_denormal = 1e-18f;
inline void kill_denormal(float &p_sample ) { p_sample += anti_denormal;p_sample -= anti_denormal;};

Module::Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1) : SEModule_base(seaudioMaster, p_resvd1)
{
	buffer = buffer2 = buffer3 = NULL;
	maBuf = maBuf2 = maBuf3 = NULL;
}

Module::~Module()
{
}

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *inP  = buffer_offset + in_buffer;
    float *l1P  = buffer_offset + l1_buffer;
    float *l2P  = buffer_offset + l2_buffer;
    float *l3P  = buffer_offset + l3_buffer;
    float *mix1P  = buffer_offset + mix1_buffer;
    float *mix2P  = buffer_offset + mix2_buffer;
    float *mix3P  = buffer_offset + mix3_buffer;
    float *feedbackP  = buffer_offset + feedback_buffer;
	float *maLengthP = buffer_offset + maLength_buffer;
    float *outP = buffer_offset + out_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float in=(*inP++) * (polarity==0 ? 1.f : -1.f);
		float l1=*l1P++;
		float l2=*l2P++;
		float l3=*l3P++;
		float mix1=*mix1P++;
		float mix2=*mix2P++;
		float mix3=*mix3P++;
		float feedback=10.f * (*feedbackP++);
		if (oversample != oversamplePrev)
			myInit(); // must reset everything!
		float maLength=((float)oversample) + MABUFSZ * (*maLengthP++);

		if (outOfMemory)
		{
			// will output noise as a warning:
			float rnd = rand()/(float)RAND_MAX;
			*outP++ = 0.1f * rnd;	// TODO: beep or popup error message?
			continue;
		}

		// interpolate change in frequency over oversampling interval
		// to avoid audible 44khz artifacts
		float out;
		int ii;
		for (ii=1 ; ii<=oversample ; ++ii)
		{
			if (l1 != l1Prev)
			{
				float l1i=l1Prev+(l1-l1Prev)*((float)ii)/((float)oversample);
				if (scale == INCHES)
					d1 = (int)(SAMPPERSEC * oversample * l1i * 2.f * 10.f / (float)INCHPERSEC);
				else
					d1 = (int)(SAMPPERSEC * oversample * l1i / 100.f);	// * 10.f / 1000.f;
				if (d1 < 0)
					d1 = 0;
			}	
			if (l2 != l2Prev)
			{
				float l2i=l2Prev+(l2-l2Prev)*((float)ii)/((float)oversample);
				if (scale == INCHES)
					d2 = (int)(SAMPPERSEC * oversample * l2i * 2.f * 10.f / (float)INCHPERSEC);
				else
					d2 = (int)(SAMPPERSEC * oversample * l2i / 100.f);	// * 10.f / 1000.f;
				if (d2 < 0)
					d2 = 0;
			}
			if (l3 != l3Prev)
			{
				float l3i=l3Prev+(l3-l3Prev)*((float)ii)/((float)oversample);
				if (scale == INCHES)
					d3 = (int)(SAMPPERSEC * oversample * l3i * 2.f * 10.f / (float)INCHPERSEC);
				else
					d3 = (int)(SAMPPERSEC * oversample * l3i / 100.f);	// * 10.f / 1000.f;
				if (d3 < 0)
					d3 = 0;
			}

			out=0.f;
			if (mode == MULTITAP)
			{
				// buffer[] is a shared ring-buffer large enough for all delays!
				float o = buffer[count];
				buffer[count] = 0.f;	// for next time through
				out = o = movAvg(o,(int)(maLength+.5f),maLengthPrev,maCount,maBuf,maSum); // cheap LP filter
				o *= feedback;
				// to avoid gaps:
				int i;
				if (mix1>0.f)
				{
					float res1 = (o+mix1*in) * .333f;
					kill_denormal(res1);
					if (d1<d1prev)
						--d1prev;	// don't add anything to buffer or you'll double-bookkeep
					else
					{
						for (i=(d1prev<0 ? d1 : d1prev) ; (d1prev<d1 ? i<=d1 : i>=d1) ; (d1prev<d1 ? ++i : --i))
							if (i>0) buffer[(count+i) % BUFSZ] += res1;
							else out += res1;
						d1prev = d1;
					}
				}
				if (mix2>0.f)
				{
					float res2 = (o+mix2*in) * .333f;
					kill_denormal(res2);
					if (d2<d2prev)
						--d2prev;	// don't add anything to buffer or you'll double-bookkeep
					else
					{
						for (i=(d2prev<0 ? d2 : d2prev) ; (d2prev<d2 ? i<=d2 : i>=d2) ; (d2prev<d2 ? ++i : --i))
							if (i>0) buffer[(count+i) % BUFSZ] += res2;
							else out += res2;
						d2prev = d2;
					}
				}
				if (mix3>0.f)
				{
					float res3 = (o+mix3*in) * .333f;
					kill_denormal(res3);
					if (d3<d3prev)
						--d3prev;	// don't add anything to buffer or you'll double-bookkeep
					else
					{
						for (i=(d3prev<0 ? d3 : d3prev) ; (d3prev<d3 ? i<=d3 : i>=d3) ; (d3prev<d3 ? ++i : --i))
							if (i>0) buffer[(count+i) % BUFSZ] += res3;
							else out += res3;
						d3prev = d3;
					}
				}
			}
			else
			{
				out = 0.f;
				int i;
				if (mix1>0.f)
				{
					float o = buffer[count];
					buffer[count] = 0.f;	// for next time through
					o = movAvg(o,(int)(maLength+.5f),maLengthPrev,maCount,maBuf,maSum); // cheap LP filter
					out += o;
					o *= feedback;
					float res1 = (o+mix1*in);
					kill_denormal(res1);
					// to avoid gaps:
					if (d1<d1prev)
						--d1prev;	// don't add anything to buffer or you'll double-bookkeep
					else
					{
						for (i=(d1prev<0 ? d1 : d1prev) ; (d1prev<d1 ? i<=d1 : i>=d1) ; (d1prev<d1 ? ++i : --i))
							if (i>0) buffer[(count+i) % BUFSZ] += res1;
							else out += res1;
						d1prev = d1;
					}
				}
				if (mix2>0.f)
				{
					float o2 = buffer2[count];
					buffer2[count] = 0.f;	// for next time through
					o2 = movAvg(o2,(int)(maLength+.5f),maLengthPrev2,maCount2,maBuf2,maSum2); // cheap LP filter
					out += o2;
					o2 *= feedback;
					float res2 = (o2+mix2*in);
					kill_denormal(res2);
					// to avoid gaps:
					if (d2<d2prev)
						--d2prev;	// don't add anything to buffer or you'll double-bookkeep
					else
					{
						for (i=(d2prev<0 ? d2 : d2prev) ; (d2prev<d2 ? i<=d2 : i>=d2) ; (d2prev<d2 ? ++i : --i))
							if (i>0) buffer2[(count+i) % BUFSZ] += res2;
							else out += res2;
						d2prev = d2;
					}
				}
				if (mix3>0.f)
				{
					float o3 = buffer3[count];
					buffer3[count] = 0.f;	// for next time through
					o3 = movAvg(o3,(int)(maLength+.5f),maLengthPrev3,maCount3,maBuf3,maSum3); // cheap LP filter
					out += o3;
					o3 *= feedback;
					float res3 = (o3+mix3*in);
					kill_denormal(res3);
					// to avoid gaps:
					if (d3<d3prev)
						--d3prev;	// don't add anything to buffer or you'll double-bookkeep
					else
					{
						for (i=(d3prev<0 ? d3 : d3prev) ; (d3prev<d3 ? i<=d3 : i>=d3) ; (d3prev<d3 ? ++i : --i))
							if (i>0) buffer3[(count+i) % BUFSZ] += res3;
							else out += res3;
						d3prev = d3;
					}
				}
			}
			++count;
			count %= BUFSZ;
		}

		*outP++ = out;
		l1Prev = l1;
		l2Prev = l2;
		l3Prev = l3;
	}
}

void Module::close()
{
	delete [] buffer;
	delete [] buffer2;
	delete [] buffer3;
	delete [] maBuf;
	delete [] maBuf2;
	delete [] maBuf3;

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

	SAMPPERSEC = getSampleRate();

	myInit();

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
	properties->name = "MultiDelay2";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG MultiDelay2.1";

	// Info, may include Author, Web page whatever
	properties->about = "MultiDelay v.2.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
May 2004 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
Quick multitap delay line, intended primarily for short, \n\
audio-frequency delays as found in acoustic musical instruments \n\
and speaker cabinets. Specify the length Ln of each \n\
dimension in inches or millisec (see the SCALE parameter). \n\
The maximum length is 6160 inches (the delay is double \n\
this: 12320 inches), per second, as specified by MAXSECS parameter. \n\
\n\
Set the FEEDBACK (0 -> 1): 0 is a single delay. You can invert \n\
the POLARITY of the delay signal to model the rearward signal \n\
of the speaker. \n\
\n\
DAMPING (0 -> 1) introduces a low-pass filter into the feedback path, \n\
simulating the absorption of high frequencies by reflective \n\
surfaces and by the air itself. The lowest cutoff is 200 Hz. \n\
\n\
OVERSAMPLE eliminates artifacts when the delay time changes, \n\
at the expense of added cpu and memory usage. If it can't \n\
allocate memory, then it will produce a loud 'hiss'. In this \n\
case you should reduce oversampling or reduce your host's \n\
memory use (freeze tracks, etc). \n\
\n\
Use the MODE parameter to use a single multitap delay or 3 \n\
independent delay lines. \n\
\n\
REVISION HISTORY \n\
24 Sep 2007: Add MAXSECS parameter for longer times.\n\
04 Mar 2006: New version added oversample.\n\
09 Jun 2005: Fix multitap behavior, speed improvements.\n";
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
		properties->flags				= IO_LINEAR_INPUT;
		break;
	case PN_L1:
		properties->name				= "L1";
		properties->variable_address	= &l1_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "14";
		properties->datatype_extra		= IPS",1,"IPS",1";
		break;
	case PN_L2:
		properties->name				= "L2";
		properties->variable_address	= &l2_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "22";
		properties->datatype_extra		= IPS",1,"IPS",1";
		break;
	case PN_L3:
		properties->name				= "L3";
		properties->variable_address	= &l3_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "32";
		properties->datatype_extra		= IPS",1,"IPS",1";
		break;
	case PN_FEEDBACK:
		properties->name				= "Feedback";
		properties->variable_address	= &feedback_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= ".6";
		properties->datatype_extra		= "1,0,1,0";
		break;
	case PN_MIX1:
		properties->name				= "Mix1";
		properties->variable_address	= &mix1_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_MIX2:
		properties->name				= "Mix2";
		properties->variable_address	= &mix2_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_MIX3:
		properties->name				= "Mix3";
		properties->variable_address	= &mix3_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_MALENGTH:
		properties->name				= "Damping";
		properties->variable_address	= &maLength_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "0";
		properties->datatype_extra		= "10,0,10,0";
		break;
	case PN_OVERSAMPLE:
		properties->name				= "Oversample";
		properties->variable_address	= &oversample;
		properties->datatype_extra		= "16x=16,8x=8,4x=4,2x=2,none=1";
		properties->default_value		= "1";
		properties->direction			= DR_IN;
		properties->datatype			= DT_ENUM;
		break;
	case PN_OUT:
		properties->name				= "Out";
		properties->variable_address	= &out_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case PN_POLARITY:
		properties->name				= "Polarity";
		properties->variable_address	= &polarity;
		properties->datatype_extra		= "normal=0,invert=1";
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_ENUM;
		properties->default_value		= "0";
		break;
	case PN_MODE:
		properties->name				= "Mode";
		properties->variable_address	= &mode;
		properties->datatype_extra		= "multitap=0,independent=1";
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_ENUM;
		properties->default_value		= "0";
		break;
	case PN_SCALE:
		properties->name				= "Scale";
		properties->variable_address	= &scale;
		properties->datatype_extra		= "inches=0,millisec=1";
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_ENUM;
		properties->default_value		= "0";
		break;
	case PN_MAXSECS:
		properties->name				= "Max Secs";
		properties->variable_address	= &maxSecs;
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_FLOAT;
		properties->default_value		= "1";
		break;
	default:
		return false; // host will ask for plugs 0,1,2,3 etc. return false to signal when done
	};

	return true;
}

void Module::myInit()
{
	l1Prev = l2Prev = l3Prev = 0.f;
	d1prev = d2prev = d3prev = d1 = d2 = d3 = 0;
	BUFSZ = 0;
	MABUFSZ = 0;
	maSum = maSum2 = maSum3 = 0.f;
	maCount = maCount2 = maCount3 = 0;
	maLengthPrev = maLengthPrev2 = maLengthPrev3 = 0;
	oversamplePrev = oversample;
	count = 0;
	outOfMemory = false;

	BUFSZ = createBuffer(buffer);
	if (BUFSZ == 0)
		{ outOfMemory = true; return; }
	BUFSZ = createBuffer(buffer2);
	if (BUFSZ == 0)
		{ outOfMemory = true; return; }
	BUFSZ = createBuffer(buffer3);
	if (BUFSZ == 0)
		{ outOfMemory = true; return; }

	 // 100 samples at 44.1k sample rate: min cutoff around 200 Hz
	MABUFSZ = createMaBuffer(maBuf);
	if (MABUFSZ == 0)
		{ outOfMemory = true; return; }
	MABUFSZ = createMaBuffer(maBuf2);
	if (MABUFSZ == 0)
		{ outOfMemory = true; return; }
	MABUFSZ = createMaBuffer(maBuf3);
	if (MABUFSZ == 0)
		{ outOfMemory = true; return; }
}

// allocate some memory to hold the delayed samples
int Module::createBuffer(FSAMPLE*& buf)
{
	int buffer_size = (int)(SAMPPERSEC * maxSecs * oversample + 1);

	delete [] buf;
	buf = new FSAMPLE[buffer_size];
	if (buf == NULL)
		return 0;

	// clear buffer
	memset(buf, 0, buffer_size * sizeof(FSAMPLE) ); // clear buffer

	return buffer_size;
}

int Module::createMaBuffer(float*& buf)
{
	int buffer_size = (int)(SAMPPERSEC*oversample/441 + 1);
	
	delete [] buf;
	buf = new float[buffer_size];
	if (buf == NULL)
		return 0;

	memset(buf, 0, buffer_size * sizeof(float) );
	return buffer_size;
}


// mod function -- C's % operator doesn't do neg like perl does
//#define MOD(i,n) ((i)>=0 ? (i)%(n) : (i)+(n))
#define MOD(i,n) ((i)>=0 ? (i) : (i)+(n))
 
// Highly efficient "boxcar" digital filter, simply
// the moving average of last n samples. This has a lot
// of stopband variation, but ok in this application where
// the output is a comb filter anyway.
// Ring-buffer contains history.
//
// To translate cutoff frequency to n samples:
//   n = SAMPPERSEC/(2*f); if (n<=1) ...
//
float Module::movAvg(float x, int n, int& maLengthPrev, int& maCount, float maBuf[], float& maSum)
{
	maBuf[maCount] = x;

	if (n<=1)
		maSum = x;
	else if (n != maLengthPrev)
	{
		maSum = 0.;
		for (int i=0 ; i<n ; ++i)
			maSum += maBuf[MOD(maCount-i,MABUFSZ)];
	}
	else
	{
		maSum += maBuf[maCount];
		maSum -= maBuf[MOD(maCount-n,MABUFSZ)];
		maSum *= .9999f;	// for stability
	}

	++maCount;
	maCount %= MABUFSZ;
	maLengthPrev = n;

	return (n<=1 ? maSum : maSum/n);
}
