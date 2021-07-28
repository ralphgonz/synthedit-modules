/*-----------------------------------------------------------------------------

© 2002, Jeff M Soft und Hardware GmbH, All Rights Reserved

-----------------------------------------------------------------------------*/

#include "Module.h"
#include "SEMod_struct.h"
#include "SEPin.h"
#include <assert.h>
#include <math.h>

#define PN_CLOCK_OUT 0
#define PN_BAR_RESET_OUT 1
#define PN_TEMPO_OUT 2
#define PN_TRANSPORT_OUT 3
#define PN_MET_OUT 4

#define DEFAULT_TEMPO 2 // 20 BPM

Module::Module(seaudioMasterCallback2 seaudioMaster, void *p_resvd1) : SEModule_base(seaudioMaster, p_resvd1)
,pulse_count(0.f)
,pulse_count_int(0)
,m_midi_clock(-1)
,quarter_note_count(-1)
,quarters_per_bar(4)
,error(0.f)
,bpm_out(this,PN_TEMPO_OUT)
,clock_out(this,PN_CLOCK_OUT)
,bar_reset_out(this,PN_BAR_RESET_OUT)
,transport_out(this,PN_TRANSPORT_OUT)
{
}

Module::~Module()
{
	// nothing to see here, move along.
}

//#define CLOCKS_PER_BEAT 24.0

void Module::sub_process(long buffer_offset, long sampleFrames )
{
//    float *out1 = buffer_offset + output1_buffer;
//    float *out2 = buffer_offset + output2_buffer;
//    float *out3 = buffer_offset + output3_buffer;
//    float *out4 = buffer_offset + output4_buffer; // bar start

	float samples_per_beat;
	VstTimeInfo *ti = 0;

//	assert(false);

	int samples_remain = sampleFrames;
	int buffer_position = buffer_offset;

	while(true)
	{
		int to_do = samples_remain;
		if( pulse_count_int < samples_remain )
		{
			to_do = pulse_count_int;
		}

		bpm_out.process( buffer_position, to_do );
		clock_out.process( buffer_position, to_do );
		bar_reset_out.process( buffer_position, to_do );
		transport_out.process( buffer_position, to_do );
/*
		for( int i = to_do ;  i > 0 ; i-- )
		{
			*out1++ = outval;
//			*out2++ = cur_tempo_val;
			*out3++ = cur_transport_running;
			*out4++ = cur_bar_start;
		}
*/
		samples_remain -= to_do;
		pulse_count_int -= to_do;

		if( samples_remain == 0 )
			return;

		buffer_position += to_do;

/*
    for( int s = sampleFrames ; s > 0 ; s-- )
    {
		pulse_count -= 1.f;
*/
		assert( pulse_count_int == 0 );
		assert( pulse_count <= 0.f );
		{
			int samples_processed_so_far = sampleFrames - samples_remain;
			unsigned int cur_sample_clock = SampleClock() + samples_processed_so_far;

			float new_bar_reset = 0.f;
			if( clock_out.GetValue() == 0.f )
			{
				// keep track of position within beat (24 clock/beat)
				m_midi_clock++;
				m_midi_clock %= pulses_per_beat;

				if( m_midi_clock == 0) // start of quarter note
				{
					quarter_note_count++;
					quarter_note_count %= quarters_per_bar;
					if( quarter_note_count == 0) // start of bar
					{
						new_bar_reset = 0.5f;
					}
				}

				clock_out.SetValue( cur_sample_clock, 0.5f );
				// RG 12/8/2004: Basically this is all I did,
				// add a "metronome" output which gives intermediate
				// values (0,1) based on on position in pulses_per_beat
				met_out = 2.f*
					(m_midi_clock < pulses_per_beat/2 ?
					((float)m_midi_clock) :
					((float)(pulses_per_beat-m_midi_clock))) /
					((float)pulses_per_beat);
				// RG: I think this is the right way to tell downstream modules about changes:
				getPin(PN_MET_OUT)->TransmitStatusChange( SampleClock() + samples_processed_so_far, ST_ONE_OFF );
			}
			else
			{
//				outval = 0.f;
				clock_out.SetValue( cur_sample_clock, 0.f );
			}

			bar_reset_out.SetValue( cur_sample_clock, new_bar_reset );

			if( ti == 0 ) // only do this a max of once per call
			{
//				float new_bar_start;


				// add on time since block start
				ti = (VstTimeInfo *) CallHost(seaudioMasterGetTime, 0, kVstTempoValid|kVstPpqPosValid, 0 );
				if( ti != 0 )
				{
					// calculate how many samples per beat/clock
					double samples_to_beats = ti->tempo / ( sampleRate * 60.0 ); // converts secs to beats (quarter notes)
					double samples_to_clocks = samples_to_beats * pulses_per_beat;

					//int samples_processed_so_far = sampleFrames - samples_remain;//s;
					// half a midi clock (for on/off transition)
					samples_per_beat = 0.5f / (float)samples_to_clocks;

					float new_transport_running = 0.f;

					// lock to host if transport playing, else just freewheel
					if( (ti->flags & kVstTransportPlaying) != 0 )
					{
						new_transport_running = 1.f;

						double extra_samples = buffer_offset + (double) samples_processed_so_far;
						double host_total_beats = ti->ppqPos + extra_samples * samples_to_beats;
						quarter_note_count = (int) host_total_beats;

						float beat_frac = (float) m_midi_clock;

						// this routine called twice per midi clock (leading edge/trailing edge)
						// on trailing edge, we're half way between midi clocks
						if( clock_out.GetValue() == 0.f )//outval == 0.f )
						{
							beat_frac += 0.5;
						}

						// add small fractional ammount due to pulse not always
						// falling on an exact sampleframe
						beat_frac -= 0.5f * pulse_count/samples_per_beat;

						// convert from MIDI clocks to beats
						beat_frac = beat_frac / (float) pulses_per_beat;

						float host_beat_frac = (float)(host_total_beats - floor( host_total_beats ));
						
						// calc error as fraction of beat
						error = host_beat_frac - beat_frac;
						if( error < -0.5 )
							error += 1.0;
						if( error > 0.5 )
							error -= 1.0;

						// convert error back to samples
						float correction = error * 2.f * samples_per_beat * pulses_per_beat;

						// jump count a fraction to catch up with host
						pulse_count -= correction; //samples_per_beat * error * 0.8f;

//#ifdef _DEBUG
//						getPin(PN_OUTPUT5)->TransmitStatusChange( SampleClock() + samples_processed_so_far, ST_ONE_OFF );
//#endif
					}
					
					float new_tempo = (float)(ti->tempo *0.1f);

					// avoid divide-by-zero errors when host don't provide tempo info
					if( new_tempo == 0.f )
						new_tempo = DEFAULT_TEMPO;

					bpm_out.SetValue      ( cur_sample_clock, new_tempo  );
					transport_out.SetValue( cur_sample_clock, new_transport_running );
				}
			}

			// reset pulse count
			pulse_count += samples_per_beat;

			// can't skip a pulse completely, that would lose sync
			if( pulse_count < 2.f )
			{
				pulse_count = 2.f;
			}

			pulse_count_int = (int) ceil(pulse_count);

			// pre-calculate pulse count at next clock
			pulse_count -= pulse_count_int;
		}
/*
		*out1++ = outval;
		*out2++ = cur_tempo_val;
		*out3++ = cur_transport_running;
		*out4++ = cur_bar_start;
*/
/*
#ifdef _DEBUG
		*out4++ = error;
#endif
*/
	}
}

void Module::open()
{
	SET_PROCESS_FUNC(Module::sub_process);
	SEModule_base::open();
	bpm_out.SetValue( SampleClock(), DEFAULT_TEMPO );
	met_out = 0.f;
//	getPin(PN_MET_OUT)->TransmitStatusChange( SampleClock(), ST_RUN );
}

// describe the module
bool Module::getModuleProperties (SEModuleProperties* properties)
{
	// describe the plugin, this is the name the end-user will see.
	properties->name = "Metronome";

	// return a unique string 32 characters max
	// if posible include manufacturer and plugin identity
	// this is used internally by SE to identify the plug. No two plugs may have the same id.
	properties->id = "REG Metronome";

	// Info, may include Author, Web page whatever
	properties->about = "Metronome v.1.0.1 \n\
Ralph Gonzalez rgonzale@ibl.bm \n\
Jan 2005 \n\
\n\
Licensing: free to use and distribute, if documentation \n\
and credits accompany. \n\
\n\
Slight modification of Jeff's Clock module for use in \n\
metronome VST.\n";
	return true;
}

// describe the pins (plugs)
bool Module::getPinProperties (long index, SEPinProperties* properties)
{
	switch( index )
	{
	case 0:
		properties->name				= "Pulse Out";
		properties->variable_address	= clock_out.PointerAddress();//&output1_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case 1:
		properties->name				= "Bar Start";
		properties->variable_address	= bar_reset_out.PointerAddress();//&output4_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case 2:
		properties->name				= "Tempo Out";
		properties->variable_address	= bpm_out.PointerAddress();
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case 3:
		properties->name				= "Transport Run";
		properties->variable_address	= transport_out.PointerAddress();//&output3_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
	case 4:
		properties->name				= "Metronome";
		properties->variable_address	= &met_out;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FLOAT;
		break;
	case 5:
		properties->name				= "Pulse Divide";
		properties->variable_address	= &pulses_per_beat;
		properties->datatype_extra		= "64=64,32=32,24=24,16=16,12=12,8=8,4=4,3=3,2=2,1=1";
		properties->default_value		= "4";
		properties->direction			= DR_IN;
		properties->datatype			= DT_ENUM;
		break;
/*
#ifdef _DEBUG
	case 4:
		properties->name				= "Diagnostic";
		properties->variable_address	= &output4_buffer;
		properties->direction			= DR_OUT;
		properties->datatype			= DT_FSAMPLE;
		break;
#endif
*/
	default:
		return false; // host will ask for plugs 0,1,2,3 etc. return false to signal when done
	};

	return true;
}
