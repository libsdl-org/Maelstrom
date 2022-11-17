
/* Possible image dump formats */
#define IMAGE_XPM	0x00
#define IMAGE_GIF	0x01

/* Define your favorite image format here */
#define IMAGE_FORMAT	IMAGE_GIF

/* Figure our file extension */
#if IMAGE_FORMAT == IMAGE_XPM
#define IMAGE_TYPE	"XPM"
#endif
#if IMAGE_FORMAT == IMAGE_GIF
#define IMAGE_TYPE	"gif"
#endif

/* The global image dump function */
/* 8-bit */
extern void ImageDump(FILE *dumpfp, int width, int height, int ncolors,
				unsigned long *new_colors, unsigned char *data);
/* > 8-bit */
extern void ImageDump(FILE *dumpfp, int width, int height, int bpp,
							unsigned char *data);
