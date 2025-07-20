#include "main.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define ADD_MEMORY  1
#define SET_STATE  2

unsigned int options = DEFAULT_OPTIONS;

/* Program parameters. */
int hn_samplerate = HN_DEFAULT_SAMPLERATE;
float hn_tau = HN_DEFAULT_TAU;
double hn_beta = HN_DEFAULT_BETA;
double hn_epsilon = HN_DEFAULT_EPSILON;

void print_help()
{
        puts("See README.md for commandline instructions.");
}

int parse_program_options(int argc, char **argv)
{
        if (argc == 1) {
                print_help();
                exit(0);
        }

        int c;
        while ((c = getopt(argc, argv, "t:r:b:e:lip:")) != -1) {
                switch (c) {
                case 't':
                        hn_tau = atof(optarg);
                        break;

                case 'r':
                        hn_samplerate = atoi(optarg);
                        break;

                case 'b':
                        hn_beta = atof(optarg);
                        break;

                case 'e':
                        hn_epsilon = atof(optarg);
                        break;

                case 'l':
                        options |= OPTION_LOG;
                        break;

                case 'i':
                        options |= OPTION_INTERACTIVE;
                        break;

                case 'p': // ps, pm
                        /* Bad hack, no error handling whatsoever. */
                        options |= *optarg == 's'? OPTION_PLAYSTATES: 0;
                        options |= *optarg == 'm'? OPTION_PLAYMEMORIES: 0;
                        break;

                case '?': // error
                        exit(1);
                }
        }

        /*
         * Return index to first non-option argument, which is either
         * a memory file, a colon or empty (in that case optind == argc).
         */
        return optind;
}

void print_program_parameters()
{
        printf("memory: %.2f sec @ %i Hz\n"
               "energy epsilon: %.2e\n",
               hn_tau, hn_samplerate, hn_epsilon);
}

Signal *extract_signal(const char *filename, long num_samples)
{
        try {
                Signal *s = new Signal(filename);

                if (s->length() > hn_tau) {
                        s->throw_exception("signal too long (%.2f sec); "
                                           "max capacity is %.2f sec",
                                           s->length(), hn_tau);
                }

                s->normalize();

                /*
                 * Resampling at small rates (< 8000 Hz) can introduce
                 * peak values slightly above or under 1.
                 */
                s->resample(num_samples, hn_samplerate);

                return s;
        }
        catch(std::exception &e) {
                fprintf(stderr, "%s: %s\n", filename, e.what());

                return NULL;
        }
}

bool add_memory_from_file(HopfieldNetwork *h, const char *filename)
{
        Signal *s = extract_signal(filename, h->get_num_neurons());

        if (s) {
                if (options & OPTION_PLAYMEMORIES) {
                        printf("playing memory file: %s\n", filename);
                        s->play();
                }

                h->add_memory(s);
                delete s;

                return true;
        }

        return false;
}

bool set_state_from_file(HopfieldNetwork *h, const char *filename)
{
        Signal *s = extract_signal(filename, h->get_num_neurons());
        if (s) {
                if (options & OPTION_PLAYSTATES) {
                        printf("playing state file: %s\n", filename);
                        s->play();
                }

                h->set_state(s);
                delete s;

                return true;
        }

        return false;
}

bool sweep_callback(HopfieldNetwork *h, double current_energy)
{
        if (options & OPTION_LOG)
                printf("sweep completed; energy = %f\n", current_energy);

        /* Play the state (testing). */
        if (options & OPTION_PLAYSTATES) {
                Signal s(h, hn_samplerate);
                s.play();
        }

        /* Returning false will case the converging to stop. */
        return true;
}

void converge_and_play_stable_state(HopfieldNetwork *h)
{
        /* Find a nearby stable state. */
        h->converge(hn_epsilon, sweep_callback);

        Signal s(h, hn_samplerate);

        puts("playing stable state");
        s.play();
}

void do_REP(HopfieldNetwork *h)
{
        char input[BUFSIZ];

        printf("path to file for initial state? ");
        gets(input);

        if (strlen(input) == 0) {
                h->set_to_random_state();
                puts("set to random state");
        } else {
                if (!set_state_from_file(h, input))
                        return;
        }

        converge_and_play_stable_state(h);
}

int main(int argc, char **argv)
{
        int memory_i = parse_program_options(argc, argv);

        long hn_I = (long) (hn_tau * hn_samplerate);
        HopfieldNetwork h(hn_I, hn_beta);

        for (; memory_i < argc; memory_i++) {
                const char *memory_file = argv[memory_i];

                /* A colon (':') separates the memory and state files. */
                if (strcmp(memory_file, ":") == 0)
                        break;

                if (add_memory_from_file(&h, memory_file))
                        printf("added memory: %s\n", memory_file);
                else
                        printf("ignored memory: %s\n", memory_file);
        }

        if (options & OPTION_LOG) {
                print_program_parameters();
                h.print_parameters();
        }

        int state_i = memory_i + 1;
        for (; state_i < argc; state_i++) {
                const char *state_file = argv[state_i];

                if (set_state_from_file(&h, state_file)) {
                        printf("set state: %s\n", state_file);
                        converge_and_play_stable_state(&h);
                }
                else
                        printf("ignored state: %s\n", state_file);
        }

        if (options & OPTION_INTERACTIVE) {
                srand(time(0));

                while (1) // REPL
                        do_REP(&h);
        }

        return 0;
}
