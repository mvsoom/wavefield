#ifndef _HOPFIELDNETWORK_H
#define _HOPFIELDNETWORK_H

#include "Signal.h"

//#define HN_WEIGHT_CHECK_BOUNDS

#define HN_TYPICAL_ENERGY_EPSILON  (1.e-6)

class HopfieldNetwork {
private:
        long I;         // number of neurons
        float *x;       // neuron states

        long W;         // number of weights
        float *w;       // network weights

        int num_memories = 0;

        double beta;

        int w2d(int i, int j);

public:
        HopfieldNetwork(long I, double beta);
        ~HopfieldNetwork();

        long get_num_neurons();

        void set_all_weights(float val);
        float get_weight(int i, int j);
        void print_weights();
        void print_parameters();

        void set_state(Signal *s);
        void set_to_random_state();
        float get_state(int i);
        int add_memory(Signal *s);

        double activation(int i);
        double update_state(int i);
        double sweep();
        double energy();
        double converge(double epsilon, bool (*callback)(HopfieldNetwork*,
                                                         double) = NULL);
};

#endif
