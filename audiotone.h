#ifndef AUDIOTONE_H_INCLUDED
#define AUDIOTONE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


int
audio_tone(unsigned short *samp, int nsamps, double *pt0, 
	   double freq, double amp, int sps);

#ifdef __cplusplus
}
#endif

#endif // AUDIOTONE_H_INCLUDED
