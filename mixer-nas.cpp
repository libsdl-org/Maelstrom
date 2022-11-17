
/* The sound server for Maelstrom!

   Much of this code has been adapted from 'sfxserver', written by
   Terry Evans, 1994.  Thanks! :)

   Again hacked by Paul Kendall for NAS. 1996 paul@pablo.kcbbs.gen.nz

   Rehacked for the latest version of nas (1.2p5) by Sam Lantinga
*/

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include <audio/audiolib.h>
#include <audio/soundlib.h>

#include "mixer.h"

extern "C" {
	extern char *getenv(char *);
};

Mixer:: Mixer(char *ignored_param, unsigned short initvolume)
{
	int i;

	device = 0;

        /* Start up with initial volume */
        DSPopen(0);     /* Init device, but don't complain */
        (void) SetVolume(initvolume);

	/* Initialize the sound sample to bucket mappings */
	/* We assume relatively few sounds, so linear search is okay */
	BucketMap_head.next = NULL;

	/* Now set up the channels */
	for ( i=0; i<NUM_CHANNELS; ++i ) {
		channels[i].in_use = 0;
		channels[i].which = i;
		channels[i].sample = NULL;
	}
}

Mixer:: ~Mixer()
{
	BucketMap *next_entry, *curr_entry;

	/* Clean out the Bucket/ID map */
	for ( curr_entry = BucketMap_head.next; curr_entry; ) {
		next_entry = curr_entry->next;
		delete curr_entry;
		curr_entry = next_entry;
	}
	DSPclose();
}


int
Mixer:: DSPopen(int complain)
{
	int i;

	if ( Device_Opened() )	/* We're already open */
		return(0);

	speed = 11025;

	/* try and connect to the NCD audio server */
	if (!(auserver=AuOpenServer(NULL, 0, NULL, 0, NULL, NULL))) {
		if ( complain ) {
			perror("Mixer: Cannot connect to NCD audio server");
		}
		return(-1);
	}

	/* Look for an audio device that we can use */
	for (i = 0; i < AuServerNumDevices(auserver); i++)
	{
		if ((AuDeviceKind(AuServerDevice(auserver, i)) == 
					AuComponentKindPhysicalOutput) && 
			AuDeviceNumTracks(AuServerDevice(auserver, i)) == 1) 
		{
			device=AuDeviceIdentifier(AuServerDevice(auserver, i));
			break;
		}
	}

	/* Well we didn't get a device - all busy? */
	if (!device) {
		if ( complain ) {
			perror("Mixer: Cannot obtain NCD audio device.");
		}
		return(-1);
	}

#if defined(SOUNDLIB_VERSION) && SOUNDLIB_VERSION >= 2
	AuSoundRestartHardwarePauses = AuFalse;
#endif
	return(0);  /* All A-OK */
}

void
Mixer:: DSPclose(void)
{
	if ( Device_Opened() ) {
		AuCloseServer(auserver);
		device = 0;
	}
}

int
Mixer:: SetVolume(unsigned short Volume)
{
	if ( Volume > 0x08 ) {
		error("Mixer: Warning: Volume is a range 0-8\n");
		return(-1);
	}
	volume = AuFixedPointFromFraction(Volume, 8);
	return(0);
}

int
Mixer:: SoundID(unsigned short channel)
{
	if ( channel > NUM_CHANNELS-1 )
		return(-1);
	if ( channels[channel].in_use )
		return(channels[channel].sample->ID);
	return(-1);
}

static void
doneCB(AuServer *aud, AuEventHandlerRec *handler, AuEvent *event, void *ch)
{
	struct Mixer::Channel *channel;

	channel = (struct Mixer::Channel *)ch;
	
	if ( channel->in_use )
		channel->in_use = 0;
	if ( channel->sample->callback )
		(channel->sample->callback)(channel->which);
}

int
Mixer:: Play_Sample(unsigned short channel, Sample *sample)
{
	BucketMap *last_entry, *curr_entry;
	AuBucketID sound_id;
	
	if ( channel > NUM_CHANNELS-1 )
		return(-1);

	/* If the sound device isn't available, callback immediately */
	if ( ! Device_Opened() ) {
		if ( sample->callback )
			sample->callback(channel);
		return(0);
	}

	/* Look up this sound ID in our bucket list */
	for ( last_entry=&BucketMap_head, curr_entry=BucketMap_head.next;
				curr_entry; curr_entry=curr_entry->next ) {
		if ( curr_entry->local_id == sample->ID )
			break;

		/* Loop updating */
		last_entry = curr_entry;
	}
	if ( ! curr_entry )  { /* We must create a bucket and map */
		Sound sound;
		sound=SoundCreate(SoundFileFormatNone,AuFormatLinearUnsigned8,
						1, speed, sample->len, NULL);

		curr_entry = new struct Mixer::BucketMap;
		last_entry->next = curr_entry;
		curr_entry->local_id = sample->ID;
		curr_entry->remote_id = AuSoundCreateBucketFromData(auserver,
					sound, sample->data, 0, NULL, NULL);
		curr_entry->next = NULL;
/*printf("Created new sound bucket for ID %d\n", sample->ID);*/
	}
	
	channels[channel].in_use = 1;
	channels[channel].sample = sample;

	/* Play sound */
	AuSoundPlayFromBucket(auserver, curr_entry->remote_id, device, volume,
		::doneCB, &channels[channel], 0, &channels[channel].flow,
							NULL, NULL, NULL);
	return(0);
}

/* Set up the I/O handler */
void
Mixer:: Setup_IO(int fd, void (*handler)(int))
{
}

void
Mixer:: Play(void)
{
	if ( Device_Opened() ) {
		AuHandleEvents(auserver);
	}
}

void
Mixer:: Halt(unsigned short channel)
{
	if ( channel > NUM_CHANNELS-1 )
		return;

	if ( channels[channel].in_use ) {
		/*AuDestroyFlow(auserver, channels[channel].flow, NULL);*/
		channels[channel].in_use = 0;
	}
}

void
Mixer:: HaltAll(void)
{
	for ( unsigned short i=0; i<NUM_CHANNELS; ++i )
		Halt(i);
}

