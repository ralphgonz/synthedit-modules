
////////////////////////////////////
// rnd automation module.
////////////////////////////////////


#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// RndAuto Copyright 2005 Ralph Gonzalez


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
	float *minRange_buffer;	// scale 0,10
	float *maxRange_buffer;	// scale 0,10
	float *freq_buffer;	// scale 0,10
	float *rise_buffer;	// scale 0,10
	float *out_buffer;

	// cached values:
	float SAMPPERSEC;
	int count;
	int cycleLen;
	int riseLen;
	float rnd;
	float prevRnd;
};
#endif
