
#ifndef _sample_h
#define _sample_h

/* The structure of the sound sample data */
typedef struct {
	int ID;
	int len;
	unsigned char *data;
	void (*callback)(unsigned short channel);
	} Sample;

#endif /* _sample_h */
