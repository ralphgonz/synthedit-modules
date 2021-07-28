/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// Static Copyright 2004 Ralph Gonzalez

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
#define PN_TAU			0
#define PN_ALPHA		1
#define PN_BETA			2
#define PN_OUT			3

#define PI 3.141592654F
#define PI2 1.570796327F

static double MYTAN[3000];
static double MYATAN[3100];
static double myTan(double x);
static double myATan(double x);
static void createTan();
static void createATan();
inline double fast_pow(double x, int y)
{
	double out=1.F;
	for (int i=0 ; i<y ; ++i)
		out *= x;
	return out;
}
 
Module::Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1) : SEModule_base(seaudioMaster, p_resvd1)
{
}

Module::~Module()
{
}

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *alphaP  = buffer_offset + alpha_buffer;
    float *betaP  = buffer_offset + beta_buffer;
    float *tauP  = buffer_offset + tau_buffer;
    float *outP = buffer_offset + out_buffer;
	for( int s = sampleFrames; s > 0 ; s-- )
	{
		double alpha=*alphaP++;
		double beta=(*betaP++) * 10.F;
		double tau=*tauP++;

		// restrict large values and values that cause div by zero:
		if (beta>15.f)
			beta = 15.f;
		else if (beta<.002f)	// avoid div by zero
			beta = .002f;
		if (alpha>1.f)
			alpha = 1.f;
		else if (alpha<0.f)
			alpha = 0.f;
		if (tau>1.f)
			tau = 1.f;
		else if (tau<.002f)
			tau = .002f;

		double x=rand()/(double)RAND_MAX;
		if (beta != prev_beta)
		{
			prev_beta = beta;
//			betaPrime=1.F-pow(2.F,-beta);
			betaPrime=1.F-1.F/fast_pow(2.F,(int)(beta+.999f));
			c=tau/myTan(PI2*betaPrime);
			alphaPrime=myATan(alpha/c)/(PI2*betaPrime);
		}
		else if (tau != prev_tau)
		{
			prev_tau = tau;
			// normalize so max=1:
			c=tau/myTan(PI2*betaPrime);
			alphaPrime=myATan(alpha/c)/(PI2*betaPrime);
		}
		else if (alpha != prev_alpha)
		{
			prev_alpha = alpha;
			// make output proportional to input:
			alphaPrime=myATan(alpha/c)/(PI2*betaPrime);
		}
		double b=alphaPrime * betaPrime;
		float out=(float)(c*myTan((x-.5F)*PI*b));

		*outP++ = out;

#ifdef DDEBUG
fprintf(fp,"%d %d %d %d %.3f\n",
	pmCount,pmPeriod,pos2,transPeriod,pmDepth);
#endif
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

	createTan();
	createATan();

	prev_beta = prev_alpha = prev_tau = -999999.F;

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
	properties->name = "Static";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG Static";

	// Info, may include Author, Web page whatever
	properties->about = "Static v.1.0.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
May 2004 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
Generates noise using the Cauchy probability distribution.\n\
This models a sparse 'static' sound. Adjust SPARSE parameter to\n\
vary the output from white noise to 'static'. The GAIN should \n\
usually remain at 10. It may be used as an input for the \n\
special case where you want the sound to 'die out' from \n\
white noise to static. \n\
\n\
Apply a gain value (eg from an envelope) to vary the RANGE\n\
parameter.\n\
";
	return true;
}

// describe the pins (plugs)
bool Module::getPinProperties (long index, SEPinProperties* properties)
{
	switch( index )
	{
	case PN_TAU:
		properties->name				= "Gain";
		properties->variable_address	= &tau_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_ALPHA:
		properties->name				= "Range";
		properties->variable_address	= &alpha_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "10";
		break;
	case PN_BETA:
		properties->name				= "Sparse";
		properties->variable_address	= &beta_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "7";
		properties->datatype_extra		= "15,0,15,0";
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

#define LIM1 1.49225651F	// PI/2 - PI/2/20
#define LIM2 1.566869336F	// PI/2 - PI/2/20/20
#define LIM3 PI2			// PI/2

#define INC1 1.570796327e-3F	// PI/2/1000
#define INC2 7.853981634e-5F	// .../20
#define INC3 3.926990817e-6F	// .../20

static int MYTANLEN;
void createTan()
{
	double x=0.F;
	int i=0;
	for ( ; x<LIM1 ; x+=INC1,++i)
		MYTAN[i] = tan(x);
	for ( ; x<LIM2 ; x+=INC2,++i)
		MYTAN[i] = tan(x);
	for ( ; x<LIM3 ; x+=INC3,++i)
		MYTAN[i] = tan(x);

	MYTANLEN = i-1;	// save this to counter roundoff error (why -1?)
}

double myTan(double x)
{
	int i;

	if (x<0.F)
		i = 0;
	else if (x<LIM1)
		i = (int)(x/INC1);
	else if (x<LIM2)
		i = 950 + (int)((x-LIM1)/INC2);
	else // if (x<LIM3)
		i = 1900 + (int)((x-LIM2)/INC3);
//	else
//		i = 2900 - 1;

	// not exactly 2900 because of roundoff error:
	if (i>=MYTANLEN)
		i = MYTANLEN - 1;

	return MYTAN[i];
}

#define INC4 1.270620477e-2F	// tan(PI/2*.95) / 1000
#define INC5 12.70620477F
#define INC6 12706.20477F
#define LIM4 INC5
#define LIM5 INC6
#define LIM6 12706204.77F

void createATan()
{
	double x=0.F;
	int i=0;
	for ( ; x<LIM4 ; x+=INC4,++i)
		MYATAN[i] = atan(x);
	for ( ; x<LIM5 ; x+=INC5,++i)
		MYATAN[i] = atan(x);
	for ( ; x<LIM6 ; x+=INC6,++i)
		MYATAN[i] = atan(x);
}

double myATan(double x)
{
	int i;

	if (x<0.F)
		i = 0;
	else if (x<LIM4)
		i = (int)(x/INC4);
	else if (x<LIM5)
		i = 1000 + (int)((x-LIM4)/INC5);
	else if (x<LIM6)
		i = 2000 + (int)((x-LIM5)/INC6);
	else
		i = 3000 - 1;

	return MYATAN[i];
}

