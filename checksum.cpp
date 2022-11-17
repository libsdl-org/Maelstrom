
/* Okay, here is a method of doing checksumming on ourselves.

   We calculate a checksum over the text segment from the address
   of main() to the end of the text segment (etext), and then 
   encrypt it and ascii armour it (base64) for transport.

   Note, this is NOT foolproof, so don't rely on it for critical software!

   It is pretty slick though. :)
   The md5 checksum is xor'd with a random value, which is stored in
   the authentication packet.  This gives relatively random input to
   the RSA encryption engine which encrypts the packet with the server's
   public key.  The result is ascii armoured and sent to the server
   where the md5 checksum is extracted.  Each time the packet is sent,
   it is a different ascii text, but decodes to a known checksum.

   This method should be resistant to cracking by snooping, spoofing,
   and code tampering, as long as the private key remains private.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include "checksum.h"
#include "myerror.h"

/* These checksum routines are activated by -DUSE_CHECKSUM */
#ifdef USE_CHECKSUM

/* RSA MD5 checksum, public key routines */
#include "../RSA/source/global.h"
#include "../RSA/source/rsaref.h"
extern "C" {
	extern int RSAPublicEncrypt (
			unsigned char *output,	/* output block */
			unsigned int *outputLen,/* length of output block */
			unsigned char *input,	/* input block */
			unsigned int inputLen,	/* length of input block */
			R_RSA_PUBLIC_KEY *publicKey,	/* RSA public key */
			R_RANDOM_STRUCT *randomStruct	/* random structure */
	);
};
#include "public_key.h"

/* Encrypt and ascii armour a message */
static char *armour_encrypt(unsigned char *buf, unsigned int len);

/* Here is where we save the checksum and encrypted checksum */
#define MD5LEN	16
static unsigned char our_checksum[MD5LEN];
static unsigned char weak_encoder;

/* Call this to calculate the checksum -- first thing in main()! */
void checksum(void)
{
	/* How many times do you see this? :) */
	extern void main(int argc, char *argv[]);

	/* These are the end of the text and data segments. */
	extern int etext, edata;

#ifdef PRINT_CHECKSUM
error("Main = 0x%x, etext = 0x%x, edata = 0x%x\n",main,&etext,&edata);
#endif
	/* Local variables */
	void *mem_end=NULL;
	int   i;
	MD5_CTX *ctx;

	/* Find the end of our code segment */
	mem_end = &etext;
	if ( (int)mem_end < (int)main ) {	// Uh oh...
		error("Warning: unexpected environment -- no checksum!!\n");
		return;
	}

	/* Allocate and calculate our checksum */
 	ctx = new MD5_CTX;
	MD5Init(ctx);
	MD5Update(ctx, (unsigned char *)main,
				(unsigned int)mem_end-(unsigned int)main);
	MD5Final(our_checksum, ctx);

/* ERASE THIS!! */
#ifdef PRINT_CHECKSUM
error("Real checksum: ");
for ( i=0; i<MD5LEN; ++i )
	error("%.2x", our_checksum[i]&0xFF);
error("\n");
#endif

	/* Use weak random encoding, to discourage hackers */
	struct timeval now;
	gettimeofday(&now, NULL);
	weak_encoder = (now.tv_usec&0xFF);
	memset(&now, 0, sizeof(now));
	for ( i=0; i<MD5LEN; ++i )
		our_checksum[i] =  our_checksum[i]^weak_encoder;
	return;
}

/* Convert to Base64 encoding */
static char to_base64[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' 
	};
#define PAD	'='		/* Trailing pad character */


void base64_encode(char **dstptr, unsigned char *from, unsigned int fromlen)
{
	char *to = *dstptr = new char[(fromlen*4)/3+5];
	unsigned char sextet[4];
	unsigned int i, index=0, o=0;

#ifdef PRINT_CHECKSUM
error("Encoding %d chars...", fromlen);
#endif
	for ( i=0; i<fromlen; ++i ) {
		switch (index++) {
			case 0: sextet[0]  = ((from[i]&0xfc)>>2);
					sextet[1]  = ((from[i]&0x03)<<4);
					break;
			case 1: sextet[1] |= ((from[i]&0xf0)>>4);
					sextet[2]  = ((from[i]&0x0f)<<2);
					break;
			case 2: sextet[2] |= ((from[i]&0xc0)>>6);
					sextet[3]  = (from[i]&0x3f);
					/* Now output the encoded data */
					for ( index=0; index<4; ++index )
						to[o++] = to_base64[sextet[index]];
					index=0;
					break;
			default:  /* Never reached */ ;
		}
	}
	if ( index ) { /* We must flush the output */
		for ( i=0; i<=index; ++i )
			to[o++] = to_base64[sextet[i]];
		for ( ; index<3; ++index )
			to[o++] = PAD;
	}
	to[o] = 0;
#ifdef PRINT_CHECKSUM
error("into %d chars. (allocated %d chars)\n", o+1, (fromlen*4)/3+5);
#endif
	return;
}

/* Okay, encryption method:
	Get a random seed, use generic random encoding on
	our message digest.  Then use the server's public key to
	RSA encrypt the encrypted message together with the seed.
	Translate this encapsulated message to ascii and we're done.
*/
static char *armour_encrypt(unsigned char *buf, unsigned int len)
{
	unsigned int i;
	struct timeval now;
	unsigned char *tmp, seed=0;
	char *encoded;

	/* Use weak random encoding, erase as we go. :-) */
	tmp = new unsigned char[++len];
	gettimeofday(&now, NULL);
	seed = (now.tv_usec&0xFF);
	memset(&now, 0, sizeof(now));
	for ( tmp[0]=seed, i=1; i<len; ++i ) {
		tmp[i] =  buf[i-1]^seed;
		buf[i-1] = 0;
	}
	seed = 0;

	/* RSA encrypt the seed+digest */
	R_RSA_PUBLIC_KEY *pkey=&public_key;
	unsigned int clen=0;
	unsigned char *cbuf = new unsigned char[MAX_ENCRYPTED_KEY_LEN];
	{
		unsigned int bytesleft; unsigned char randbyte;
		R_RANDOM_STRUCT weewee;

		/* Initialize silly random struct */
		R_RandomInit(&weewee);
		gettimeofday(&now, NULL);
		srand(now.tv_usec);
		for ( R_GetRandomBytesNeeded(&bytesleft, &weewee);
						bytesleft > 0; --bytesleft ) {
			randbyte = (rand()%256);
			R_RandomUpdate(&weewee, &randbyte, 1);
		}
			
		/* Get down to business! */
		if (RSAPublicEncrypt(cbuf, &clen, tmp, len, pkey, &weewee)) {
			/* Uh oh... what do we do? */
			error("Warning!  RSA encryption failed!\n");
			clen = 0;
		}
	}
	/* Clear out the original buffer, just in case */
	for ( i=0; i<len; tmp[i++]=0 );

	/* Now ascii encode it */
#ifdef PRINT_CHECKSUM
error("Encoding: ");
for ( i=0; i<clen; ++i )
	error("%.2x", cbuf[i]&0xFF);
error("\n");
#endif
	base64_encode(&encoded, cbuf, clen);

	/* Clean up and return */
	for ( i=0; i<clen; cbuf[i++]=0 );
	delete[] cbuf;
	return(encoded);
}

/* Call this later, when you want to see the checksum */
char *get_checksum(unsigned char *key, int keylen)
{
	unsigned char csum[MD5LEN], seed;
	int i, j;
	char *encap_csum;

	memcpy(csum, our_checksum, MD5LEN);
	if ( keylen ) {
		for ( i=0, j=0; i<MD5LEN; ++i ) {
			seed = key[j++];
			j %= keylen;
			csum[i] ^= weak_encoder;
			csum[i] ^= seed;
		}
	} else {
		for ( i=0, j=0; i<MD5LEN; ++i )
			csum[i] ^= weak_encoder;
	}
	encap_csum = armour_encrypt(csum, MD5LEN);
	memset(csum, 0, MD5LEN);
	
	return(encap_csum);
}

#else  /* Don't use checksumming */
static inline void Unused(...) { }	/* For eliminating compiler warnings */

void checksum(void) { return; }

char *get_checksum(unsigned char *key, int keylen)
{
	Unused(key); Unused(keylen);
	static char *foo = "Checksum Not Enabled";
	return(foo);
}
#endif /* USE_CHECKSUM */
