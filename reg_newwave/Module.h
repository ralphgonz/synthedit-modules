#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// NewWave Copyright 2004 Ralph Gonzalez

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
	float *minin_buffer;	// scale 0,10v
	float *trans_buffer;	// scale -4,4v
	float *pmDepth_buffer;	// scale 0,10v
	float *pmRatio_buffer;	// scale 0,10v
	float *out_buffer;
	float *outF_buffer;
	float *outA_buffer;
	short wave;
	float hyster;

	// cached values:
	int period;
	int transPeriod;
	int count;
// TODO: use continuous frequency in place of period, 
// interpolate between period and newPeriod each cycle
	float freq;
//	float newF;
	int pmCount;
	int pmPeriod;
	float amp;
	float prevAmp;
	int pendingPeriod;
	int newCount;
	float newAmp;
	float pendingAmp;
	int state;
	bool wentNeg;
	float trans;
	float prevRawTrans;
	int prevPos;
	int downPos;
	int downPeriod;

	// almost constants:
	int SAMPLEFREQ;
	int MAXSIN;

	float waveform(short wave, int count, int period);
	float* mySin;
};
#endif
