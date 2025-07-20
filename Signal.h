#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <stdlib.h>

class HopfieldNetwork;

//#define SIGNAL_SAMPLE_CHECK_BOUNDS

/*
 * The signal is hardcoded in float values, not via template types.
 * Note that the simple API of libsamplerate only accepts float values.
 */
class Signal {
private:
        long N;                  // number of samples
        int samplerate;
        float *x = NULL;         // samples

public:
        Signal(const char *filename);
        Signal(HopfieldNetwork *h, int samplerate);
        ~Signal();

        void print();
        void throw_exception(const char *format, ...);

        long num_samples();
        float length();
        float get_peak();
        float normalize();
        void resample(long new_N, int new_samplerate);

        float get_sample(long);
        void write(const char *filename);
        int play();
};

#endif
