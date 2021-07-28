#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// Enveloper Copyright 2003 Ralph Gonzalez

class Module : public SEModule_base
{
public:
	Module(seaudioMasterCallback seaudioMaster);
	~Module();

	virtual bool getModuleProperties (SEModuleProperties* properties);
	virtual bool getPinProperties (long index, SEPinProperties* properties);
	virtual void sub_process(long buffer_offset, long sampleFrames );
	void OnPlugStateChange(SEPin *pin);
	void CreateBuffer(float width, float nopumpWidth);
	virtual void open();
	virtual void close();

private:
	float *input_buffer;
	float *deadZone_buffer;
	float *width_buffer;
	float *nopump_buffer;
	float *output_buffer;
	float *preroll_buffer;
	short mode;
	short math;

	int buffer_size;
	int buf_pos;	// ring buffer offset
	int nopumpSize;
	int nopumpCount;
	bool latchUp;
	float sum;
	float max;
	int maxpos;
	int count;
	int recalcCount;
	bool fullCount;
	bool rampingUp;
	bool rampingDown;
	int rampCount;
	float prevWidth;
	float prevNopumpWidth;
	FSAMPLE * buffer;
};
#endif
