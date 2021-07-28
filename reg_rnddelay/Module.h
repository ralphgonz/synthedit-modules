#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// Random Delay Copyright 2003 Ralph Gonzalez

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
	float *gate_buffer; // pointer to circular buffer of samples
	float *input_buffer;
	float *gain_buffer;
	float *delay_buffer; // [0,1] desired mean relative to max delay
	float *delay_width_buffer; // [0,1] relative width of random delay
	float *gain_width_buffer; // [0,1] relative width of random gain
	float *pulse_width_buffer;
	float *output_buffer;
	FSAMPLE * buffer;
	short unitIn;
	int sample_count;
	int loop_count;
	int fade_count;
	int fade_in_count;
	int fade_pos;
	int buffer_size;
	int max_buffer_size;
	int loop_size;
	int fade_size;
	int fade_in_size;
	float buffer_ms;
	float fade_ms;
	float fade_in_ms;
	float gdelta;
	float gdelta_next;
	float fade_gdelta;
	bool gate_open;
	bool sampling;
	bool looping;
	bool fading;

	void getWidths(float d,float dw,float gw,int* dp,float* gp);
};
#endif
