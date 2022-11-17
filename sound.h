
/* This is the client stub module for Maelstrom Sound */

#include "sound_cmds.h"

#ifdef __WIN95__
#include "Win95/soundaux.h"
#endif

class SoundClient {

public:
	SoundClient(void);
	~SoundClient();

	int  SetVolume(unsigned short vol);
	int  LoadSound(short sndID);
	int  PlaySound(short sndID, short priority, 
					void (*STCallBack)(unsigned short));
	int  PlayChannel(short sndID, short priority,
		unsigned short theChannel, void (*STCallBack)(unsigned short));
	void HaltSounds(void);
	void HaltChannel(unsigned short theChannel);
	int  IsSoundPlaying(short sndID);
	void ChannelStatus(unsigned short *chanOnePriority,
			   unsigned short *chanTwoPriority,
			   unsigned short *chanThreePriority,
			   unsigned short *chanFourPriority);

private:
	struct channel {
		short  ID;
		short  priority;
#ifdef __WIN95__
		LPDIRECTSOUNDBUFFER buffer;
#else
		int    in_use;
		void (*Callback)(unsigned short);
#endif
		} channels[NUM_CHANNELS];

  	int  Status(unsigned short channel);
	int  SoundID(unsigned short channel);

#ifdef __WIN95__
	SoundRes      *sounds;			/* The Macintosh sounds */
	short          volume;			/* The current volume */
	static const int volumeTable[9];// Perceptually linear volume lookup

	Hash<SoundSample> *SS_Hash;		/* Hash load list of sounds */
	void RestoreBuffers(void);		/* Restore sound buffers */

#else /* UNIX */

	friend void IO_Handler(int sig);
	int  QueryServer(void);
	void SoundEvent(void);			/* Sound Event handler */
	int  SendNGetReply(char *cmdbuf, int timeout);	/* Wait for OKAY_CMD */
	void CallBack(unsigned short channel);

	int lightning_fd;			/* Asynchronous I/O channel */
	int galumph_fd;				/* Stop-wait I/O channel */
	int sndserver_pid;

	struct Q_ent {
		unsigned short channel;
		struct Q_ent  *next;
		} Callback_Q, *Callback_Qtail;
	int awaiting_reply;

	inline void QueueCallback(unsigned short channel) {
		Callback_Qtail->next = new struct SoundClient::Q_ent;
		Callback_Qtail = Callback_Qtail->next;
		Callback_Qtail->channel = channel;
		Callback_Qtail->next = NULL;
	}
	inline void DequeueCallbacks(void) {
		struct SoundClient::Q_ent *oldptr, *ptr;

		awaiting_reply = 0;
		for ( ptr=Callback_Q.next; ptr; ) {
			CallBack(ptr->channel);
			oldptr = ptr;
			ptr = ptr->next;
			delete oldptr;
		}
		Callback_Qtail = &Callback_Q;
		Callback_Q.next = NULL;
	}
#endif /* Win95 */
};
