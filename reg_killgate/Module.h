#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// Killgate Copyright 2003 Ralph Gonzalez

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
	float *gate_buffer;	// if not connected (-1), self-gates input
	float *output_buffer;
	bool gate_open;
	bool outIsStatic;
	float prevOut;
	float killval;
	short mode;
};
#endif
