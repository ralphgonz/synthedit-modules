#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// FMLFO Copyright 2004 Ralph Gonzalez

class Module : public SEModule_base
{
public:
	Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1);
	~Module();

	static bool getModuleProperties (SEModuleProperties* properties); 
	virtual bool getPinProperties (long index, SEPinProperties* properties);
	void SE_CALLING_CONVENTION sub_process(long buffer_offset, long sampleFrames );
	void OnPlugStateChange(SEPin *pin);
	void CreateBuffer();
	virtual void open();
	virtual void close();

private:
	float *in_buffer;	// scale -10,10V
	float *depth_buffer;	// scale 0,10v
	float *offset_buffer;	// scale 0,10v
	float *freq_buffer;	// scale 0.0001,20v
	float *phase_buffer;	// scale 0,360v
	float *out_buffer;
	short wave;
	short fscale;
	short dscale;

	float movAvg(float x, int n, int& maLengthPrev, int& maCount, float maBuf[], float& maSum);
	float maSum;
	int maCount;	// for ring-buffer
	int maLengthPrev;
	int maLength;
	int MAMAX;
	float* maBuf;

	// cached values:
	int prevPeriod;
	int pos;

	// almost constants:
	int SAMPLEFREQ;
	int MAXSIN;

	float waveform(short wave, int count, int period);
	float* mySin;
};
#endif
