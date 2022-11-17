
/* Maelstrom Sound Mixer class

	-Sam Lantinga			(5/5/95)
*/

#ifdef _SGI_SOURCE
#include <bstring.h>
#endif
#ifdef _AIX
#include <sys/select.h>
#endif

#include "sound_cmds.h"
#include "sample.h"
#include "mydebug.h"


/* Include system specific audio header files */
#include <audio/audiolib.h>
#include <audio/soundlib.h>

/* Woah!  The CLASS! :) */
class Mixer {

public:
	Mixer(char *device, unsigned short initvolume);
	~Mixer();

	int  SetVolume(unsigned short Volume);
	int  SoundID(unsigned short channel);
	int  Play_Sample(unsigned short channel, Sample *sample);
	void Play(void);
	void Halt(unsigned short channel);
	void HaltAll(void);
	void Setup_IO(int fd, void (*handler)(int));

private:
        int  DSPopen(int complain);     // Open the sound device
        void DSPclose(void);            // Close the sound device

	AuServer *auserver;		// The audio server connection
	AuDeviceID device;		// The audio device ID

	int   speed;			// The recording speed of the samples

	AuFixedPoint volume;		// Current volume of playing samples

	/* Structure to map local samples to remote buckets */
	struct BucketMap {
		int local_id;
		AuBucketID remote_id;
		struct Mixer::BucketMap *next;
		} BucketMap_head;

	/* Some channels and the sound data to play */
	struct Channel {
		int   in_use;		// Is this channel in use?
		int   which;		// Which channel number is this?
		AuFlowID flow;
		Sample *sample;		// Current sample on this channel
	} channels[NUM_CHANNELS];

	int Device_Opened(void) {
		return(device != 0);
	}
};
