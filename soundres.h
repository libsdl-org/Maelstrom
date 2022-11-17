
/* This is the sound resource --> sound sample module for Maelstrom */

#include "Mac_Resource.h"
#include "sample.h"

/* Macintosh sound definitions:						 */

typedef struct {
	unsigned long D_offset;		/* Offset of sampled sound */
	unsigned long n_samples;	/* Number of sound samples */
	unsigned long sample_rate;	/* Sample recording rate */
	unsigned long Loop_start;	/* Beginning of sound loop */
	unsigned long Loop_end;		/* End of sound loop */
	unsigned char encoding;		/* Sound encoding */
	unsigned char freq_base;	/* Base freq of recording */

	unsigned long init_op;		/* Initialization for DSP device */
	unsigned char *samples;
	} Snd_Sample;
	
/****************************************************************/

class SoundRes {

public:
	SoundRes(char *resfile);
	~SoundRes();

	int     LoadSound(int ID);
	Sample *GetSound(int ID);
	void    FreeSound(int ID);

private:
	int           Load_Sound(struct Mac_ResData *D, Snd_Sample *S);
	Mac_Resource *soundres;
	short        *id2samplei;
	Sample      **Sounds;
	unsigned int  lastsound;
};
