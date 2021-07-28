#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include <stdio.h>
#include "SEModule_base.h"

// PrintChart Copyright 2003 Ralph Gonzalez

class Module : public SEModule_base
{
public:
	Module(seaudioMasterCallback seaudioMaster);
	~Module();

	virtual bool getModuleProperties (SEModuleProperties* properties);
	virtual bool getPinProperties (long index, SEPinProperties* properties);
	virtual void sub_process(long buffer_offset, long sampleFrames );
	void OnPlugStateChange(SEPin *pin);
	virtual void open();
	virtual void close();

private:
	float *input_buffer;
	float *input2_buffer;
	float *input3_buffer;
	float *input4_buffer;
	float width;
	char* fname;

	int period;
	float sRate;
	float secs;
	int count;
	FILE* fp;
};
#endif
