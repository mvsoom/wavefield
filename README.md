# wavefield

For fun and educational purpose; I missed C style programming.

This program stores audio fragments as memories in a continuous Hopfield network (CHN), based on Chapter 42 in MacKay (2005).

The sound library used is [`libsndfile`](http://www.mega-nerd.com/libsndfile/api.html).

## Details: implementation of the CHN

The CHN consists of $I$ neurons that form a complete directed graph with symmetrical weights $w_{ij} = w_{ji}$. The weight $w_{ij}$ denotes the connection from neuron $j$ to neuron $i$, i.e. $w_{ij} = w(i \leftarrow j)$. The weight matrix is denoted $W$.

A neuron $i$ has a state
$$ x_i \in [-1,1] $$
which depends on its activation $a_i = \sum_j w_{ij} x_j$ by the sigmoid activation function with gain $\beta$:
$$ x_i = \tanh(\beta a_i), $$ where $\beta \in [0, \inf[$.

The weights of the CHN encode the set of $N$ memories $\{x_i^{(n)}\}$ by the Hebbs rule:
$$ w_{ij} \propto \sum_{n=1}^N x_i^{(n)} x_j^{(n)}. $$
The Hebbs rule implies that **we can add new memories to the CHN on the fly**.

**Updating the weights.** The convergence of the CHN depends on symmetric weights and on the state updates being made *asynchronously* [@MacKay2005, p. 508]. This means that one neuron $i$ at a time calculates its activation $a_i$ and updates its state $x_i$. The sequence of neurons doing this may be fixed or random.