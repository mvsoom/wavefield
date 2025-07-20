#include <stdio.h>
#include <assert.h>

#include <stdexcept>

/*
 * Both these headers define *_CHECK_BOUNDS symbols in getter functons;
 * turn these off for a speed gain.
 */
#include "Signal.h"
#include "HopfieldNetwork.h"

#define HN_DEFAULT_SAMPLERATE  ((int)4000)
#define HN_DEFAULT_TAU  ((float)3.)
#define HN_DEFAULT_BETA  ((double)1.)
#define HN_DEFAULT_EPSILON ((double)1.e-12)

#define DEFAULT_OPTIONS  0x0

#define OPTION_LOG  0x1
#define OPTION_INTERACTIVE  0x2
#define OPTION_PLAYSTATES  0x4
#define OPTION_PLAYMEMORIES  0x8
