Several C++ projects for building audio/music effects. Originally for use with Synthedit SDK to assemble VST plugins, but probably adaptable to other platforms.

### BADHEAD v1.0

Tube amp simulation with variable clipping hardness,
bias (from symmetrical to rectified), and power supply regulation.

### ENVELOPER v1.01

Performs 3 related functions:
1. Flexible envelope follower, with choice of peak, absolute-value, and RMS averaging
2. Gate generator using de-noised audio input
3. De-noise filter

### KILLGATE v1.0

A little hack that outputs the input until a gate
signal appears, then outputs a 'kill' value -- optionally delayed
by one sample or lasting only one sample. Useful to feed the 'release'
inputs of David Haupt's Env Seg module.

### MULTIDELAY v1.0

Multitap delay with filter. Primarily for simulating
the resonances within acoustic instruments and speaker cabinets, but
can be used as a general-purpose delay up to 1 second in length.

### NEWWAVE v1.0

Replaces each cycle of input waveform with (transposed)
synthetic waveform. More responsive than most frequency-discovery
systems, but has additional Frequency/Amplitude outputs if you want
to drive external oscillators or filters.

### PRINTCHART v1.0

A debugging aid that samples up to 4 inputs and
periodically prints to a text file.

### QUADRATIC v1.0

Multivariate polynomial expression: `A + Bx + Cyz`.
Much less flexible than Unkargherth's U-MathEv, but efficient and
useful for simple sums, differences, and products of inputs.

### RANDOM DELAY v1.0

Samples audio input following gate signal, then plays random-length portions of the buffer to give the effect of an echo with random delay times.

