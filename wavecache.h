# pragma once

#include <cstdlib>
#include <iostream>

#include "pianokeys.h"
#include "constants.h"

using std::cout;
using std::endl;

// IMPROVE: If these ever need to be made on the fly we should use a slab allocator

// A WaveGenerator takes in a float in the range [0, 1) and returns the height of the wave
// at that position, where the wavelength is 1.
typedef float (*WaveGenerator)( float );


// TODO: this does not work! It makes artifacts at the loop point!
// New approach: super-sample the wave for one wavelength and store it for each wave shape.
//     then for each piano frequency precompute the stride length of the super-sampled array
//     by striding we should be able to hide the artifacts as a small pitch shift instead
//     of an audible "blip"
const size_t CACHED_SAMPLES = 4 * SAMPLE_RATE + 1;

struct StrideCache {
    size_t stride[ NUM_KEYS ];

    size_t getStride( size_t key ) {
        return stride[ key ];
    }

    StrideCache() {
        for ( size_t i = 0; i < NUM_KEYS; i++ ) {
           stride[i] = pianoKeyFreqs[i] * CACHED_SAMPLES / SAMPLE_RATE;
        }
    }

} strideCache;

// The goal of this struct is to precompute the waveform arrays for all piano notes
// into a cache so that we can easily use them at runtime with little overhead
struct WaveCache {
    float samples[ CACHED_SAMPLES ];

    WaveCache( WaveGenerator gen ) {
        for ( size_t i = 0; i < CACHED_SAMPLES; i++ ) {
            samples[i] = gen( i / (float)CACHED_SAMPLES );
        }
    }
};


// Used to read from a wave cache.
// It stores state between invocations
struct CacheReader {
    const WaveCache * cache;
    size_t stride;
    size_t idx;

    float readNext() {
        float ret = cache->samples[idx];
        idx += stride;
        if ( idx > CACHED_SAMPLES ) [[unlikely]] {
            idx -= CACHED_SAMPLES;
        }
        return ret;
    }

    CacheReader() = default;

    CacheReader( WaveCache & cache, size_t key )
        : cache( &cache ), stride( strideCache.getStride( key ) ), idx( 0 ) {}

    void update( WaveCache & wc, size_t key ) {
        cache = &wc;
        stride = strideCache.getStride( key );
        idx = 0;
    }
};
