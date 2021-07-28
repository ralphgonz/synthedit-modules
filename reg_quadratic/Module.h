#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// Quadratic Copyright 2003 Ralph Gonzalez

class Module : public SEModule_base
{
public:
	Module(seaudioMasterCallback seaudioMaster);
	~Module();

	virtual bool getModuleProperties (SEModuleProperties* properties);
	virtual bool getPinProperties (long index, SEPinProperties* properties);
	virtual void sub_process(long buffer_offset, long sampleFrames );
	void OnPlugStateChange(SEPin *pin);
	void CreateBuffer();
	virtual void open();
	virtual void close();

private:
	float *x_buffer;	// scale 10V
	float *y_buffer;	// scale 10v
	float *z_buffer;	// scale 10v
	float a;			// scale 1.0
	float b;			// scale 1.0
	float c;			// scale 1.0
	float *output_buffer;
};
#endif
