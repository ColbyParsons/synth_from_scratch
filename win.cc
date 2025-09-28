#define WIN32_LEAN_AND_MEAN // Removes some junk from windows headers for faster compilation

#include <windows.h>
#include <mmsystem.h>

#include <conio.h>
#include <math.h>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "constants.h"
#include "logging.h"
#include "wavecache.h"

using std::cout;
using std::endl;

#define TWOPI (M_PI + M_PI)

#define PRINT_ERROR(err) DEBUG_LOG( "Error: " << err << "\n  Function: " << __FUNCTION__ << "\n  File: " <<  __FILE__ << "\n  Line: " << __LINE__ << endl );

HWAVEOUT wave_out;

const int16_t DEFAULT_AMPLITUDE = 32767;

// We use two chunks and swap them to double-buffer the output to the underlying audio device
WAVEHDR header[2] = {{},{}};
int16_t chunks[2][CHUNK_SIZE];
size_t currChunk = 0;

unsigned long currSample = 0;

float frequency = 261.6256;

size_t currKey = 40; // Middle C

void incrementCurrSample() {
	currSample++;
	if ( currSample > SAMPLE_RATE ) {
		currSample = 0;
	}
}

// Return position in current wavelength for a qiven frequency normalized to [0, 1)
float getWavePosForFreq( float freq, size_t sample ) {
	const float wavesSoFar = sample * freq / SAMPLE_RATE;
	return wavesSoFar - (long)wavesSoFar;
}

// Uses current wavePos and returns a sin wave sample in the range [-1,1]
float getSinSample( float wavePos ) {
	return sin( TWOPI * wavePos );
}

float getSquareSample( float wavePos ) {
	return wavePos < 0.5 ? 1 : -1;
}

// Interestingly, the direction of the saw tooth doesn't seem to affect the sound
float getSawtoothSample( float wavePos ) {
	// Just map [0, 1] to [-1, 1]
	return wavePos * 2 - 1;
}

float getTriangleSample( float wavePos ) {
	// Map [0, 0.5) to [-1, 1] and [0.5, 1) to [1, -1]
	// by mapping [0, 1) to [-2, 2) and then using abs and vertical shift
	return std::abs( wavePos * 4 - 2 ) - 1;
}

// TODO: separate key handling into a second thread and have the audio thread read from
// a data structure of currently active cache readers

WaveCache triCache( getTriangleSample );

CacheReader cacheReaders[3];

void updateCacheReaders() {
	cacheReaders[0].update( triCache, currKey );
	// cacheReaders[1].update( triCache, currKey + 4 );
	// cacheReaders[2].update( triCache, currKey + 7 );
}

float currRandSample;
float lastSampledPos = -1;
float getRandSample( float baseFreq, float updateFreq, size_t sample ) {
	const float wavePos = getWavePosForFreq( baseFreq, sample );
	// If we last sampled more than updateFreq ago, or we've started a new wave, get a new rand sample
	if ( lastSampledPos < wavePos - updateFreq / (float)SAMPLE_RATE ||
		 lastSampledPos > wavePos ) {
		currRandSample = ( rand() / (float) RAND_MAX ) * 2 - 1;
	}

	lastSampledPos = wavePos;
	return currRandSample;
}

// Collection point to swap between underlying samples
int16_t getSample() {
	// float sum = 0;
	// for ( size_t i = 0; i < 3; i++ ) {
	// 	sum += cacheReaders[i].readNext();
	// }
	// sum /= 3;

	// return sum * DEFAULT_AMPLITUDE;
	return cacheReaders[0].readNext() * DEFAULT_AMPLITUDE;
}

void CALLBACK WaveOutProc(HWAVEOUT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

void setKey( size_t key ) {
	currKey = key;
	updateCacheReaders();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	((void)hInstance);
	((void)hPrevInstance);
	((void)lpCmdLine);
	((void)nShowCmd);

	initDebug();
	updateCacheReaders();

	{
		WAVEFORMATEX format = {};
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 1;
		format.nSamplesPerSec = SAMPLE_RATE;
		format.wBitsPerSample = 16; // We have signed 16 bit int representation for amplitude
		format.cbSize = 0;
		format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

		if(waveOutOpen(&wave_out, WAVE_MAPPER, &format, (DWORD_PTR)WaveOutProc, (DWORD_PTR)NULL, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
			PRINT_ERROR( "waveOutOpen failed" );
			return -1;
		}
	}

	// Higher order 2 bytes are for right channel volume (Fs in 0xFFFF0000)
	// Lower order bits are for left channel volume
	if( waveOutSetVolume(wave_out, 0xFFFFFFFF) != MMSYSERR_NOERROR ) {
		PRINT_ERROR( "waveOutGetVolume failed" );
		return -1;
	}

	for(int i = 0; i < 2; ++i) {
		for(size_t j = 0; j < CHUNK_SIZE; ++j) {
			chunks[i][j] = getSample();
		}
		header[i].lpData = (CHAR*)chunks[i];
		header[i].dwBufferLength = CHUNK_SIZE * 2;
		if(waveOutPrepareHeader(wave_out, &header[i], sizeof(header[i])) != MMSYSERR_NOERROR) {
			PRINT_ERROR( "waveOutPrepareHeader[" << i << "] failed" );
			return -1;
		}
		if(waveOutWrite(wave_out, &header[i], sizeof(header[i])) != MMSYSERR_NOERROR) {
			PRINT_ERROR( "waveOutWrite[" << i << "] failed" );
			return -1;
		}
	}

	bool quit = false;
	while( !quit ) {
		char c = _getche();
		switch(c) {
			case 'p': {
				if ( currKey != 87 ) {
					currKey++;
					updateCacheReaders();
				}
			} break;
			case 'q': {
				if ( currKey != 0 ) {
					currKey--;
					updateCacheReaders();
				}
			} break;
			case 'a': {
				setKey( 36 );
			} break;
			case 'z': {
				setKey( 37 );
			} break;
			case 's': {
				setKey( 38 );
			} break;
			case 'x': {
				setKey( 39 );
			} break;
			case 'c': {
				setKey( 40 );
			} break;
			case 'f': {
				setKey( 41 );
			} break;
			case 'v': {
				setKey( 42 );
			} break;
			case 'g': {
				setKey( 43 );
			} break;
			case 'b': {
				setKey( 44 );
			} break;
			case 'n': {
				setKey( 45 );
			} break;
			case 'j': {
				setKey( 46 );
			} break;
			case 'm': {
				setKey( 47 );
			} break;
			case 'k': {
				setKey( 48 );
			} break;
			case ',': {
				setKey( 49 );
			} break;
			case 'l': {
				setKey( 50 );
			} break;
			case '.': {
				setKey( 51 );
			} break;
			case '/': {
				setKey( 52 );
			} break;
			case 27: { // escape
				quit = true;
			} break;
		}
	}

	return 0;
}

void CALLBACK WaveOutProc(HWAVEOUT wave_out_handle, UINT message, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2) {
	((void)wave_out_handle);
	((void)instance);
	((void)param1);
	((void)param2);

	switch( message ) {
		case WOM_CLOSE:
			cout << "WOM_CLOSE" << endl;
			break;
		case WOM_OPEN:
			cout << "WOM_OPEN" << endl;
			break;
		case WOM_DONE:{
			DEBUG_LOG( "CALLED" )
			// cout << "WOM_DONE" << endl;
			for(size_t i = 0; i < CHUNK_SIZE; ++i) {
				chunks[currChunk][i] = getSample();
			}
			if(waveOutWrite(wave_out, &header[currChunk], sizeof(header[currChunk])) != MMSYSERR_NOERROR) {
				PRINT_ERROR( "waveOutWrite failed" );
			}
			currChunk = (currChunk + 1) % 2;
		} break;
	}
}
