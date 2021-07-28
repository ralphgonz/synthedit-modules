#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"

// Static Copyright 2004 Ralph Gonzalez

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
	float *alpha_buffer;	// scale 0,10V
	float *beta_buffer;	// scale 0,25v
	float *tau_buffer;	// scale 0,10v
	float *out_buffer;

	double prev_beta;
	double prev_tau;
	double prev_alpha;
	double betaPrime;
	double alphaPrime;
	double c;
};
#endif
