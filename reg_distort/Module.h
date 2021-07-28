#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// HyperDistort Copyright 2004 Ralph Gonzalez

class Module : public SEModule_base
{
public:
	Module(seaudioMasterCallback seaudioMaster);
	~Module();

	virtual bool getModuleProperties (SEModuleProperties* properties);
	virtual bool getPinProperties (long index, SEPinProperties* properties);
	virtual void sub_process(long buffer_offset, long sampleFrames );
	virtual void sub_process_static(long buffer_offset, long sampleFrames );
	void OnPlugStateChange(SEPin *pin);
	void CreateBuffer();
	virtual void open();
	virtual void close();
	void initVals();

private:
	float *in_buffer;	// scale 10V
	float *gain_buffer;	// scale 10v
	float *bias_buffer;	// scale 10v
	float *clip_buffer;	// scale 10v
	float *speed_buffer;// scale 10v
	float *cap_buffer;	// scale 10v
	float *out_buffer;
	short shape;

	int static_count;

	// cached values:
	short prevShape;
	float prevX0;
	float dcOffset;
	float dcOffsetPos;
	float dcOffsetNeg;
	float xAdj;
	float charge;
	float prevCap;
	float cap2;
	float prevClip;
	float clip2;
	float prevGain;
	float gain2;
};
#endif
