#include "bklib/audiotone.h"

#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979
#endif // M_PI

int
audio_tone(unsigned short *samp, int nsamps, double *pt0, 
	   double freq, double amp, int sps) {
    int i;
    double samp_time = *pt0;
    
    for(i=0; i<nsamps; i++) {
	samp[i] = (unsigned char)
	    ((double)(1<<15) + sin(samp_time*freq*2*M_PI)*amp);
	samp_time += 1.0/sps;
    }
    *pt0 = samp_time;
    return nsamps;
}
