#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// MultiDelay Copyright 2004 Ralph Gonzalez

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
	int createBuffer(FSAMPLE*& buffer);
	int createMaBuffer(float*& buffer);
	void myInit();
	float movAvg(float x, int n, int& maLengthPrev, int& maCount, float maBuf[], float& maSum);

private:
	float *in_buffer;	// scale -10,10V
	float *l1_buffer;	// scale 1,10000
	float *l2_buffer;	// scale 1,10000
	float *l3_buffer;	// scale 1,10000
	float *mix1_buffer;
	float *mix2_buffer;
	float *mix3_buffer;
	float *feedback_buffer;	// scale 0,1
	float *maLength_buffer;	// scale 0,10
	float *out_buffer;
	short polarity;
    short mode;
	short scale;

	// cached values:
	int d1;
	int d2;
	int d3;
	int d1prev;
	int d2prev;
	int d3prev;
	float l1Prev;
	float l2Prev;
	float l3Prev;
	int count;
	FSAMPLE * buffer;
	FSAMPLE * buffer2;
	FSAMPLE * buffer3;
	int BUFSZ;
	float SAMPPERSEC;
	float maSum;
	float maSum2;
	float maSum3;
	int maCount;	// for ring-buffer
	int maCount2;	// for ring-buffer
	int maCount3;	// for ring-buffer
	int maLengthPrev;
	int maLengthPrev2;
	int maLengthPrev3;
	int MABUFSZ;
	float* maBuf;
	float* maBuf2;
	float* maBuf3;
	short oversample;
	short oversamplePrev;
	bool outOfMemory;
	float maxSecs;
};
#endif
