
////////////////////////////////////
// movavg module
// This will have a low-pass effect very efficiently,
// if a bit uneven in the stopband. Takes a frequency
// input.
////////////////////////////////////


#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// Sumgate Copyright 2005 Ralph Gonzalez


class Module : public SEModule_base
{
public:
	Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1);
	~Module();

	static bool getModuleProperties (SEModuleProperties* properties); 
	virtual bool getPinProperties (long index, SEPinProperties* properties);
	void SE_CALLING_CONVENTION sub_process(long buffer_offset, long sampleFrames );
	void OnPlugStateChange(SEPin *pin);
	virtual void open();
	virtual void close();

private:
	float *gate_buffer;	// scale 0,10V
	float *width_buffer;
	float *out_buffer;

	float prev_gate;
	float out;
	float SAMPPERSEC;
};
#endif
