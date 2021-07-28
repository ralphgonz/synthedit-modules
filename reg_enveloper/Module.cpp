/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/
// Enveloper Copyright 2003 Ralph Gonzalez

#include "Module.h"
#include "SEMod_struct.h"
#include "SEPin.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>

// instead of refering to the pins by number, give them all a name
#define PN_INPUT	0
#define PN_DEADZONE	1
#define PN_WIDTH	2
#define PN_NOPUMP	3
#define PN_OUTPUT	4
#define PN_PREROLL	5
#define PN_MATH		6
#define PN_MODE		7

// How often to do a non-optimized average computation to
// avoid propogating arithmetic errors:
#define RECALC		1000

Module::Module(seaudioMasterCallback seaudioMaster) : SEModule_base(seaudioMaster), buffer(0)
{
}

Module::~Module()
{
}

// rign buffer:
#define BP(x) (buffer[(buf_pos+(x)) % buffer_size])

// do the work!
void Module::sub_process(long buffer_offset, long sampleFrames )
{
    float *inP  = buffer_offset + input_buffer;
	float *dz = buffer_offset + deadZone_buffer;
	float *widthP = buffer_offset + width_buffer;
	float *nopumpP = buffer_offset + nopump_buffer;
    float *outputP = buffer_offset + output_buffer;
    float *prerollP = buffer_offset + preroll_buffer;

	for( int s = sampleFrames; s > 0 ; s-- )
	{
		float in=*inP++;
		float deadZone=*dz++;
		float width=*widthP++ * 10.f;
		float nopumpWidth=*nopumpP++ * 10.f;
		float out=0.;
		float prerollOut=0.;

		if (prevWidth==0.f || prevWidth!=width || prevNopumpWidth!=nopumpWidth)
		{
			CreateBuffer(width, nopumpWidth);
			prevWidth = width;
			prevNopumpWidth = nopumpWidth;
		}

		float first=(float)fabs(BP(0));
		buf_pos = (buf_pos + 1) % buffer_size;	// increment ring buffer

		BP(buffer_size-1) = in;
		--maxpos;

		if (count < buffer_size)
			++count;
		else
		{
			prerollOut = BP(0);
			++recalcCount;
			if (math==0)	// PEAK
			{
				if (maxpos<0)	// must search entire buffer
				{
					recalcCount = 0;
					max = 0.;
					maxpos = 0;
					for (int i=0 ; i<buffer_size ; ++i)
						if ((float)fabs(BP(i)) > max)
						{
							max = (float)fabs(BP(i));
							maxpos = i;
						}
					if (max==0.)
						maxpos = buffer_size-1;
				}
				else	// only check max and new point
				{
					if (fabs(in) > max)
					{
						max = (float)fabs(in);
						maxpos = buffer_size-1;
					}
				}
				out = max;
			}
			else if (math==1)	// AVG(ABS)
			{
				if (!fullCount || recalcCount==RECALC)	// must avg entire buffer
				{
					recalcCount = 0;
					sum = 0.;
					for (int i=0 ; i<buffer_size ; ++i)
						sum += (float)fabs(BP(i));
					fullCount = true;
				}
				else	// only check buffer boundaries
				{
					sum -= first;
					sum += (float)fabs(in);
				}
				out = (float)(sum/buffer_size);
			}
			else	// RMS
			{
				if (!fullCount || recalcCount==RECALC)	// must avg entire buffer
				{
					recalcCount = 0;
					sum = 0.;
					for (int i=0 ; i<buffer_size ; ++i)
						sum += (float)(BP(i)*BP(i));
					fullCount = true;
				}
				else	// only check buffer boundaries
				{
					sum -= first*first;
					sum += (float)(in*in);
				}
				out = (float)(sqrt(sum/buffer_size));
			}

			if (rampingUp)
			{
				if (rampCount++ >= buffer_size)
					rampingUp = false;
			}
			else if (rampingDown)
			{
				if (rampCount++ >= buffer_size)
					rampingDown = latchUp = false;
			}

			if (out > deadZone)
			{
				if (!latchUp && (mode==0 || mode==2))
				{
					rampingUp = true;
					rampingDown = false;	// in case you're not done
					rampCount = 1;
				}
				latchUp = true;
				nopumpCount = 0;
			}
			else if (latchUp && !rampingDown)
			{
				++nopumpCount;
				if (nopumpCount < nopumpSize)
					;
				else
				{
					if (mode==0 || mode==2)
					{
						rampingDown = true;
						rampingUp = false;		// in case you're not done
						rampCount = 1;
					}
					else
						latchUp = false;
				}
			}
			
			if (!latchUp)
				out = 0.;
			else if (mode==0)	// continuous envelope follower
				;
			else				// latch up on signal detection
				out = 1.f;

			// ramp up/down at signal boundaries (mode 0 or 2) to avoid clicks:
			if (rampingUp)
				out *= (float)(rampCount/(float)buffer_size);
			else if (rampingDown)
				out *= (float)(1. - rampCount/(float)buffer_size);
		}

		*outputP++ = out;
		*prerollP++ = prerollOut;
	}
}

void Module::close()
{
	if (buffer)
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

	sum=0.f;
	count=0;
	max=0.f;
	maxpos=0;
	fullCount = false;
	recalcCount = 0;
	nopumpCount = 0;
	latchUp = false;
	rampingUp = rampingDown = false;
	buf_pos = 0;
	prevWidth = prevNopumpWidth = 0.f;

	// let 'downstream' modules know audio data is coming
	getPin(PN_OUTPUT)->TransmitStatusChange( SampleClock(), ST_RUN );
	getPin(PN_PREROLL)->TransmitStatusChange( SampleClock(), ST_RUN );
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
	properties->name = "Enveloper2";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG Enveloper2";

	// Info, may include Author, Web page whatever
	properties->about = "Enveloper v.2.0.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
July 2003 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
This module implements an envelope follower with noise gate. Applications \n\
include: creating an envelope from an analog input, creating a gate from \n\
an analog input, and cleanly de-noising an input signal. \n\
\n\
The envelope calculates one of the following over a WIDTH \n\
interval (default 1 ms): peak, average absolute value, or RMS \n\
RMS (root mean square) of the input signal. If this value exceeds \n\
the noise threshold (DENOISE=.0001 by default) then one of the \n\
following values is output: continuous envelope, rectangle \n\
(useful as a gate signal), or trapezoid with leading and trailing \n\
edge of width WIDTH to avoid transient clicks (useful to make a \n\
denoiser). \n\
\n\
A secondary PREROLL output actually gives a WIDTH-delayed \n\
copy of the input signal. This can be used in place of the \n\
input signal to give the effect of eliminating latency (delay) \n\
between the input and the envelope output (caused by the WIDTH \n\
of the envelope calculation). \n\
\n\
If you multiply the envelope output by the PREROLL output \n\
and use a trapezoid shape, you have a *denoiser*. \n\
\n\
The NOPUMP parameter provides some hysteresis in case the \n\
signal drops below the noise threshold briefly and then resumes. \n\
By tuning this parameter such dropouts will not result in the \n\
rectangle or trapezoid output closing to zero, which would otherwise \n\
result in a 'pumping' sound. \n\
\n\
Revision History \n\
---------------- \n\
2004 June 10, v.1.01: Preroll output updates properly now. \
2005 March 31, v.2.0: Enveloper2, made a new version to expose \
  width and nopump parameters as inputs. \
2005 June 3, v.2.0.1: rebuild with speed optimizations \
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
	case PN_DEADZONE:
		properties->name				= "Denoise";
		properties->variable_address	= &deadZone_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= ".01";
		break;
	case PN_WIDTH:
		properties->name				= "Width ms";
		properties->variable_address	= &width_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "1";
		break;
	case PN_NOPUMP:
		properties->name				= "Nopump ms";
		properties->variable_address	= &nopump_buffer;
		properties->direction			= DR_IN;
		properties->datatype			= DT_FSAMPLE;
		properties->default_value		= "100";
		break;
	case PN_OUTPUT:
		properties->name				= "Output";
		properties->variable_address	= &output_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case PN_PREROLL:
		properties->name				= "Preroll";
		properties->variable_address	= &preroll_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case PN_MATH:
		properties->name				= "Math";
		properties->variable_address	= &math;
		properties->datatype_extra		= "peak=0,avgabs=1,rms=2";
		properties->direction			= DR_PARAMETER;
		properties->datatype			= DT_ENUM;
		properties->default_value		= "0";
		break;
	case PN_MODE:
		properties->name				= "Mode";
		properties->variable_address	= &mode;
		properties->datatype_extra		= "continuous=0,rectangle=1,trapezoid=2";
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
// also recalc windows
void Module::CreateBuffer(float width, float nopumpWidth)
{
	buffer_size = (int) ( getSampleRate() * width / 1000.f );
	if( buffer_size < 1 )
		buffer_size = 1;

	if( buffer_size > getSampleRate() * 1 )	// limit to 1 s sample
		buffer_size = (int)getSampleRate() * 1;

	if (buffer)
		delete [] buffer;
	buffer = new FSAMPLE[buffer_size];

	// clear buffer
	memset(buffer, 0, buffer_size * sizeof(FSAMPLE) ); // clear buffer

	nopumpSize = (int)(getSampleRate() * nopumpWidth / 1000.f);
}

