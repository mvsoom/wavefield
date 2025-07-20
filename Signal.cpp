#include "Signal.h"
#include "HopfieldNetwork.h"
#include <math.h>
#include <stdarg.h>
#include <sndfile.h>
#include <samplerate.h>
#include <assert.h>

#include <stdexcept>

#define BUFSIZE  512
#define MAX_ERRMSG_LEN  512

/* See <http://www.mega-nerd.com/SRC/api_misc.html#Converters>. */
#define RESAMPLER_CONVERTER_TYPE  SRC_SINC_BEST_QUALITY

Signal::Signal(const char *filename)
{
        SF_INFO sfinfo;
        SNDFILE *f;

        sfinfo.format = 0;
        if (!(f = sf_open(filename, SFM_READ, &sfinfo))) {
                const char *sf_error = sf_strerror(NULL);
                this->throw_exception("read error: %s", sf_error);
        }

        if (sfinfo.channels != 1) {
                sf_close(f);
                this->throw_exception("found %i channels; only 1 supported",
                                      sfinfo.channels);
        }

        /*
         * Since the file has only one channel, each frame consists
         * of exactly one sample.
         */
        this->N = (long) sfinfo.frames;
        this->samplerate = sfinfo.samplerate;
        this->x = new float[N];

        sf_count_t samples_read = sf_readf_float(f, this->x, sfinfo.frames);
        sf_close(f);

        if (samples_read < sfinfo.frames) {
                const char *sf_error = sf_strerror(f);
                this->throw_exception("read error: %s", sf_error);
        }
}

Signal::Signal(HopfieldNetwork *h, int samplerate)
{
        this->N = h->get_num_neurons();
        this->samplerate = samplerate;

        this->x = new float[this->N];
        for (int i = 0; i < this->N; i++)
                this->x[i] = h->get_state(i);
}

Signal::~Signal()
{
        if (this->x) delete x;
}

void Signal::print()
{
        printf("number of samples: %ld\n"
               "samplerate       : %i\n"
               "length (sec)     : %f\n"
               "peak value       : %f\n",
               this->N, this->samplerate,
               this->length(), this->get_peak());
}

void Signal::throw_exception(const char *format, ...)
{
        char buffer[MAX_ERRMSG_LEN];

        va_list args;
        va_start(args, format);
        vsnprintf(buffer, MAX_ERRMSG_LEN, format, args);
        va_end(args);

        throw std::runtime_error(buffer);
}

long Signal::num_samples()
{
        return this->N;
}

float Signal::length()
{
        return ((float) this->N) / this->samplerate;
}

float Signal::get_peak()
{
        float peak = 0.;
        for (long i = 0; i < this->N; i++)
                peak = fmaxf(peak, fabs(this->x[i]));

        return peak; // always >= 0
}

float Signal::normalize()
{
        float c = this->get_peak();
        assert(c != 0);

        for (long i = 0; i < this->N; i++)
                this->x[i] /= c;

        return c;
}

void Signal::resample(long new_N, int new_samplerate)
{
        SRC_DATA data;

        data.data_in = this->x;
        data.data_out = new float[new_N];

        data.input_frames = this->N;
        data.output_frames = new_N;

        data.src_ratio = ((double) new_samplerate) / this->samplerate;

        int v;
        if ((v = src_simple(&data, RESAMPLER_CONVERTER_TYPE, 1)) != 0) {
                const char *src_error = src_strerror(v);
                this->throw_exception("resampling error: %s", src_error);
        }

        /* Pad unused output samples with zeros. */
        for (long i = data.output_frames_gen; i < new_N; i++) {
                data.data_out[i] = 0.;
        }

        /* Now morph into the resampled signal. */
        this->N = new_N;
        this->samplerate = new_samplerate;

        delete this->x;
        this->x = data.data_out;
}

float Signal::get_sample(long n)
{
#ifdef SIGNAL_SAMPLE_CHECK_BOUNDS
        assert(0 <= n && n < this->N);
#endif

        return this->x[n];
}

void Signal::write(const char *filename)
{
        SF_INFO sfinfo;
        SNDFILE *f;

        sfinfo.samplerate = this->samplerate;
        sfinfo.channels = 1;
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

        if (!(f = sf_open(filename, SFM_WRITE, &sfinfo))) {
                const char *sf_error = sf_strerror(NULL);
                this->throw_exception("open file for writing error: ",
                                      sf_error);
        }

        sf_count_t samples_written = sf_writef_float(f, this->x, this->N);

        if (samples_written < this->N) {
                const char *sf_error = sf_strerror(f);
                sf_close(f);

                this->throw_exception("write error: ", sf_error);
        }

        sf_close(f);
}

int Signal::play()
{
        char tmp_file[BUFSIZE], cmd[BUFSIZE];
        tmpnam(tmp_file);

        this->write(tmp_file);

        /* Wait until aplay has exited. */
        snprintf(cmd, BUFSIZE, "aplay --quiet \"%s\"", tmp_file);
        int v = system(cmd);

        remove(tmp_file);
        return v;
}
