
/* Okay, here is a method of doing checksumming on ourselves.

   Note, this is NOT foolproof, so don't rely on it for critical software!
*/

/* Call this to calculate the checksum -- first thing in main()! */
extern void checksum(void);

/* Call this later, when you want to see the checksum */
extern char *get_checksum(unsigned char *key, int keylen);
