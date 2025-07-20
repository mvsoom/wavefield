#include "HopfieldNetwork.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

HopfieldNetwork::HopfieldNetwork(long I, double beta)
{
        assert(I > 1 && beta != 0.);

        this->I = I;
        this->x = new float[I];

        /*
         * The weight matrix is implemented as a 1D array since it is
         * symmetrical. Note: we could save I floats more because we know
         * that the diagonal is zero everywhere.
         */
        this->W = I * (I + 1) / 2;
        this->w = new float[W];

        this->beta = beta;

        /*
         * Initialize weights to zero. This ensures that (1) self-edges are
         * forbidden and (2) add_memory() functions properly.
         */
        this->set_all_weights(0.);
}

HopfieldNetwork::~HopfieldNetwork()
{
        delete this->x;
        delete this->w;
}

int HopfieldNetwork::w2d(int i, int j)
{
        /* Convert the 2D index accessing the w matrx to a 1D index. */
#ifdef HN_WEIGHT_CHECK_BOUNDS
        assert(0 <= i && i < this->I &&
               0 <= j && j < this->I);
#endif

        if (i <= j)
                return i * this->I - (i - 1) * i / 2 + j - i;
        else
                return j * this->I - (j - 1) * j / 2 + i - j;
}

long HopfieldNetwork::get_num_neurons()
{
        return this->I;
}

void HopfieldNetwork::set_all_weights(float val)
{
        for (int i = 0; i < this->W; i++)
                this->w[i] = val;
}

float HopfieldNetwork::get_weight(int i, int j)
{
        return this->w[this->w2d(i, j)];
}

void HopfieldNetwork::print_weights()
{
        for (int i = 0; i < this->I; i++) {
                for (int j = 0; j < this->I; j++)
                        printf("%.2f ", this->get_weight(i, j));
                putc('\n', stdout);
        }
}

void HopfieldNetwork::print_parameters()
{
        printf("number of neurons: I = %ld\n"
               "number of memories: %i\n"
               "beta: %.2e\n",
               this->I, this->num_memories, this->beta);
}

void HopfieldNetwork::set_state(Signal *s)
{
        assert(this->I == s->num_samples());

        for (long i = 0; i < this->I; i++)
                this->x[i] = s->get_sample(i);
}

void HopfieldNetwork::set_to_random_state()
{
        /* Set each neuron state to a random number in [-1, 1]. */
        for (int i = 0; i < this->I; i++)
                this->x[i] = (((double) rand()) / RAND_MAX) * 2. - 1.;
}

float HopfieldNetwork::get_state(int i)
{
        return this->x[i];
}

int HopfieldNetwork::add_memory(Signal *s)
{
        assert(this->I == s->num_samples());

        for (int j = 0; j < this->I; j++) {
                for (int i = 0; i < j; i++) {
                        float x_i = s->get_sample(i),
                              x_j = s->get_sample(j);

                        this->w[this->w2d(i, j)] += x_i * x_j;
                }
        }

        return ++this->num_memories;
}

double HopfieldNetwork::activation(int i)
{
        double a_i = 0.;
        for (int j = 0; j < this->I; j++) {
                a_i += this->x[j] * this->get_weight(i, j);
        }

        return a_i;
}

double HopfieldNetwork::update_state(int i)
{
        return this->x[i] = tanh(this->activation(i));
}

double HopfieldNetwork::sweep() // O(I^2)
{
        /* Perform asynchronous update of the I neuron states. */
        for (int i = 0; i < this->I; i++) {
                this->update_state(i);
        }

        return this->energy();
}

double binary_entropy_e(double q) // base e
{
        /*
         * This function is defined mathematically for q in [0, 1].
         * In real life however q can be slightly out of this range;
         * therefore we perform some highly non-optimized cushioning.
         *
         * Note that we are misusing DBL_EPSILON somewhat.
         */
        if (q <= 0. + DBL_EPSILON)
                return 0.;
        if (q >= 1 - DBL_EPSILON)
                return 0.;

        return -q * log(q) - (1 - q) * log(1 - q);
}

double HopfieldNetwork::energy() // O(I^2)
{
        double matrix_term = 0., entropy_term = 0.;

        for (int i = 0; i < this->I; i++) {
                double x_i = this->x[i];

                entropy_term += binary_entropy_e((1. + x_i) / 2.);

                /*
                 * The product x_j w_ij w_i can probably be optimized
                 * because w is symmetrical and zero on the diagonal.
                 */
                for (int j = 0; j < this->I; j++) {
                        double x_j = this->x[j];

                        matrix_term += this->get_weight(i, j) * x_i * x_j;
                }
        }

        matrix_term *= -1./2;
        entropy_term *= -1./this->beta;

        return matrix_term + entropy_term;
}

double HopfieldNetwork::converge(double epsilon,
                                 bool (*callback)(HopfieldNetwork*, double))
{
        /*
         * Try to remember added memories, starting from current state and
         * writing subsequent states directly to internal state memory until
         * a stable state is found.
         *
         * The convergence stops when the energy of consecutive sweeps differ
         * less percentage-wise than epsilon.
         *
         * Note: is the energy proportional to I or beta? If so, what is a
         * proper magnitude for epsilon?
         */
        double new_energy = this->energy(), last_energy;
        do {
                last_energy = new_energy;
                new_energy = this->sweep();

                if (callback) // default value is NULL
                        if (!callback(this, new_energy))
                                break;

        } while (fabs((new_energy - last_energy)/last_energy) >= epsilon);

        return new_energy;
}
