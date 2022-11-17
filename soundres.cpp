
#include <fcntl.h>
/* This module converts Macintosh sound resources into sound samples
   for Maelstrom. :)

	-Sam Lantinga			(slouken@devolution.com)
*/

#include <stdlib.h>
#include <string.h>
#include "soundres.h"
#include "bitesex.h"

/*************************************************************************/
/* Macintosh sound definitions:						 */
/*************************************************************************/

/* Different sound header formats */
#define FORMAT_1	0x0001
#define FORMAT_2	0x0002

/* The different types of sound data */
#define SAMPLED_SND	0x0005

/* Initialization commands */
#define MONO_SOUND	0x00000080
#define STEREO_SOUND	0x000000A0

/* The different sound commands; we only support BUFFER_CMD */
#define SOUND_CMD	0x8050		/* Different from BUFFER_CMD? */
#define BUFFER_CMD	0x8051

/* Different original sampling rates */
#define rate44khz	0xAC440000		/* 44100.0 */
#define rate22khz	0x56EE8BA3		/* 22254.5 */
#define rate11khz	0x2B7745D0		/* 11127.3 */
#define rate11khz2	0x2B7745D1		/* 11127.3 (?) */

#define stdSH	0x00
#define extSH	0xFF
#define cmpSH	0xFE

#if defined(linux) && !defined(sparc)
#define copy1(dst, srcptr) \
		dst = *srcptr
#define copy2(dst, srcptr) \
   		*((short *)&dst) = *((short *)srcptr)
#define copy4(dst, srcptr) \
   		*((long *)&dst) = *((long *)srcptr)
#else
#define copy1(dst, srcptr) \
		dst = *srcptr
#define copy2(dst, srcptr) \
   		memcpy(&dst, srcptr, 2)
#define copy4(dst, srcptr) \
   		memcpy(&dst, srcptr, 4)
#endif

/*************************************************************************/
/* The Routines... :)                                                   */
/*************************************************************************/

SoundRes:: SoundRes(char *resfile)
{
	int    num_sounds;
	short *id_array;
	int    i;

	soundres = new Mac_Resource(resfile);
	if ( (num_sounds=soundres->get_num_resources("snd ")) < 0 ) {
		error("SoundRes: No sound resources in '%s'\n", resfile);
		exit(255);
	}
	Sounds = new Sample*[num_sounds];
	lastsound = 0;
	id_array = new short[num_sounds];
	soundres->get_resource_ids("snd ", id_array);
	id2samplei = new short[id_array[num_sounds-1]+1];
	for ( i=0; i<(id_array[num_sounds-1]+1); ++i )
		id2samplei[i] = (-1);
	delete[] id_array;
}

SoundRes:: ~SoundRes()
{
	delete   soundres;
	delete[] id2samplei;
	delete[] Sounds;
}

int
SoundRes:: LoadSound(int ID)
{
	struct Mac_ResData D;
	Snd_Sample         S;
	int                index;

	if ( soundres->get_resource("snd ", ID, &D) < 0 ) {
		error("SoundRes: Couldn't get sound for ID = %d\n", ID);
		return(-1);
	}
	if ( Load_Sound(&D, &S) < 0 ) {
		error("SoundRes: Bad sound: %s (ID = %d)\n",
		    		soundres->get_resource_name("snd ", ID), ID);
		return(-1);
	}
	delete[]  D.data;

	index = id2samplei[ID] = lastsound++ ;
	Sounds[index] = new Sample;
	Sounds[index]->ID = ID;
	Sounds[index]->len = S.n_samples;
	Sounds[index]->data = S.samples;
	Sounds[index]->callback = NULL;
	return(0);
}

Sample *
SoundRes:: GetSound(int ID)
{
	int index;

	/* Make sure we have loaded the sound */
	if ( (index=id2samplei[ID]) < 0 )
		return(NULL);
	return(Sounds[index]);
}

void
SoundRes:: FreeSound(int ID)
{
	int i, index;
	Sample *old;

	/* Make sure we have loaded the sound */
	if ( (index=id2samplei[ID]) < 0 )
		return;

	/* Now shuffle out the old sound */
	old = Sounds[index];
	for ( i=index; i<(lastsound-1); ++i )
		Sounds[i] = Sounds[i+1];

	delete[] old->data;
	delete   old;
}

/* This routine takes a Macintosh sound resource, and extracts sound
   data, and returns 0 
   If sampled sound data cannot be extracted, we return -1.
*/
int
SoundRes:: Load_Sound(struct Mac_ResData *D, Snd_Sample *S)
{
	unsigned char *data = D->data;

	unsigned short version;
	copy2(version, data); data += 2;
	bytesexs(version);

	if ( version == FORMAT_1 ) {
		unsigned short n_types;		/* Number of sound data types */
		unsigned short f_type;		/* First sound data type */
		unsigned long  init_op;		/* Initialization option */

		copy2(n_types, data); data += 2;
		bytesexs(n_types);
		if ( n_types != 1 ) {
			error("Sound: Multi-type sound!\n");
			return(-1);
		}
		copy2(f_type, data); data += 2;
		bytesexs(f_type);
		if ( f_type != SAMPLED_SND ) {
			error("Sound: Not a sampled sound resource!\n");
			return(-1);
		}
		copy4(init_op, data); data += 4;
		bytesexl(init_op);
		S->init_op = init_op;
	} else if ( version == FORMAT_2 ) {
		unsigned short ref_cnt;		/* Unused */
	
		copy2(ref_cnt, data); data += 2;
	} else {
		error("Sound: Unknown format: 0x%x\n", version);
		return(-1);
	}

	/* Next is the Sound commands section */
	{
		unsigned short num_cmds;	/* Number of sound commands */
		unsigned short command;		/* The first sound command */
		unsigned short param1;		/* BUFFER_CMD parameter 1 */
		unsigned long  param2;		/* Offset to sampled data */

		copy2(num_cmds, data); data += 2;
		bytesexs(num_cmds);
		if ( num_cmds != 1 ) {
			error("Sound: Multi-command sound!\n");
			return(-1);
		}
		copy2(command, data); data += 2;
		bytesexs(command);
		if ( (command != BUFFER_CMD) && (command != SOUND_CMD) ) {
			error("Sound: Unknown sound command: 0x%x\n",
								command);
			return(-1);
		}
		copy2(param1, data); data += 2;
		/* Param1 is ignored (should be 0x0000) */

		copy4(param2, data); data += 4;
		bytesexl(param2);
		/* Set 'data' to the offset of the sampled data */
		if ( param2 > D->length ) {
			error("Sound: Corrupt sound?\n");
			return(-1);
		}
		data = D->data+param2;
	}

	/* Next is the sampled sound header */
	{
		copy4(S->D_offset, data); data += 4;
		bytesexl(S->D_offset);
		if ( S->D_offset != 0 ) {
			error("Sound: Samples don't follow header!\n");
			return(-1);
		}
		copy4(S->n_samples, data); data += 4;
		bytesexl(S->n_samples);
		copy4(S->sample_rate, data); data += 4;
		bytesexl(S->sample_rate);
		copy4(S->Loop_start, data); data += 4;
		bytesexl(S->Loop_start);
		copy4(S->Loop_end, data); data += 4;
		bytesexl(S->Loop_end);
		/* Sound loops are ignored for now */
		S->encoding = *data++;
		if ( S->encoding != stdSH ) {
			error("Sound: Non-standard sound encoding!\n");
			return(-1);
		}
		S->freq_base = *data++;
		/* Frequency base might be used later */
		
		/* Now allocate room for the sound */
		if ( S->n_samples > D->length-(data-D->data) ) {
			error("Sound: Truncated sound!\n");
			return(-1);
		}
		S->samples = new unsigned char[S->n_samples];
		memcpy(S->samples, data, S->n_samples);
	}
	return(0);
}
