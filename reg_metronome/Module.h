#if !defined(_SEModule_h_inc_)
#define _SEModule_h_inc_

#include "SEModule_base.h"
#include "smart_output.h"

class Module : public SEModule_base
{
public:
	Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1);
	~Module();

	static bool getModuleProperties (SEModuleProperties* properties); 
	virtual bool getPinProperties (long index, SEPinProperties* properties);
	void SE_CALLING_CONVENTION sub_process(long buffer_offset, long sampleFrames );
	void open();

private:
	smart_output bpm_out;
	smart_output clock_out;
	smart_output bar_reset_out;
	smart_output transport_out;
	float met_out;
//	smart_output met_out;
	float error;
	float pulse_count;
	int pulse_count_int;
	int m_midi_clock;
	int quarter_note_count;
	int quarters_per_bar;
	short pulses_per_beat;
};

#endif
