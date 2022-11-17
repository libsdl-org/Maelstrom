
/*
   The file is written by Lee Jones.  Distribution is unlimited.
*/

/* applefile.h - Data structures used by AppleSingle/AppleDouble
 * file format
 *
 * Written by Lee Jones, 22-Oct-1993
 *
 * For definitive information, see "AppleSingle/AppleDouble
 * Formats for Foreign Files Developer's Note"; Apple Computer
 * Inc.; (c) 1990.
 *
 * Other details were added from:
 *   Inside Macintosh [old version], volumes II to VI,
 *   Apple include files supplied with Think C 5.0.1,
 *   Microsoft MS-DOS Programmer's Reference, version 5, and
 *   Microsoft C 6.00a's dos.h include file.
 *
 * I don't have ProDOS or AFP Server documentation so related
 * entries may be a bit skimpy.
 *
 * Edit history:
 *
 * when       who  why
 * ---------  ---  ------------------------------------------
 * 22-Oct-93  LMJ  Pull together from Inside Macintosh,
 *                 Developer's Note, etc
 * 26-Oct-93  LMJ  Finish writing first version and list
 *                 references
 * 06-Feb-94  EEF  Very minor cleanup
 */

/* Following items define machine specific size (for porting). */

typedef char            xchar8;         /* 8-bit field */
typedef char            schar8;         /* signed 8-bit field */
typedef unsigned char   uchar8;         /* unsigned 8-bit field */
typedef short           xint16;         /* 16-bit field */
typedef unsigned short  uint16;         /* unsigned 16-bit field */
typedef long            xint32;         /* 32-bit field */
typedef long            sint32;         /* signed 32-bit field */
typedef unsigned long   uint32;         /* unsigned 32-bit field */

/* REMINDER: the Motorola 680x0 is a big-endian architecture! */

typedef uint32 OSType;                  /* 32 bit field */

/* In the QuickDraw coordinate plane, each coordinate is
 * -32767..32767. Each point is at the intersection of a
 * horizontal grid line and a vertical grid line.  Horizontal
 * coordinates increase from left to right. Vertical
 * coordinates increase from top to bottom. This is the way
 * both a TV screen and page of English text are scanned:
 * from top left to bottom right.
 */

struct Point /* spot in QuickDraw 2-D grid */
{
    xint16 v; /* vertical coordinate */
    xint16 h; /* horizontal coordinate */
}; /* Point */

typedef struct Point Point;

/* See older Inside Macintosh, Volume II page 84 or Volume IV
 * page 104.
 */

struct FInfo /* Finder information */
{
    OSType fdType; /* File type, 4 ASCII chars */
    OSType fdCreator; /* File's creator, 4 ASCII chars */
    uint16 fdFlags; /* Finder flag bits */
    Point  fdLocation; /* file's location in folder */
    xint16 fdFldr; /* file 's folder (aka window) */
}; /* FInfo */

typedef struct FInfo FInfo;

/*
 * Masks for finder flag bits (field fdFlags in struct
 * FInfo).
 */

#define F_fOnDesk       0x0001 /* file is on desktop (HFS only) */
#define F_maskColor     0x000E /* color coding (3 bits) */
/*                      0x0010 /* reserved (System 7) */
#define F_fSwitchLaunch 0x0020 /* reserved (System 7) */
#define F_fShared       0x0040 /* appl available to multiple users */
#define F_fNoINITs      0x0080 /* file contains no INIT resources */
#define F_fBeenInited   0x0100 /* Finder has loaded bundle res. */
/*                      0x0200  /* reserved (System 7) */
#define F_fCustomIcom   0x0400 /* file contains custom icon */
#define F_fStationary   0x0800 /* file is a stationary pad */
#define F_fNameLocked   0x1000 /* file can't be renamed by Finder */
#define F_fHasBundle    0x2000 /* file has a bundle */
#define F_fInvisible    0x4000 /* file's icon is invisible */
#define F_fAlias        0x8000 /* file is an alias file (System 7) */

/* See older Inside Macintosh, Volume IV, page 105.
 */

struct FXInfo /* Extended finder information */

{
    xint16 fdIconID; /* icon ID number */
    xint16 fdUnused[3]; /* spare */
    schar8 fdScript; /* scrip flag and code */
    schar8 fdXFlags; /* reserved */
    xint16 fdComment; /* comment ID number */
    xint32 fdPutAway; /* home directory ID */
}; /* FXInfo */

typedef struct FXInfo FXInfo;

/* Pieces used by AppleSingle & AppleDouble (defined later). */

struct ASHeader /* header portion of AppleSingle */
{
    /* AppleSingle = 0x00051600; AppleDouble = 0x00051607 */
    uint32 magicNum; /* internal file type tag */
    uint32 versionNum; /* format version: 2 = 0x00020000 */
    uchar8 filler[16]; /* filler, currently all bits 0 */
    uint16 numEntries; /* number of entries which follow */
}; /* ASHeader */

typedef struct ASHeader ASHeader;

struct ASEntry /* one AppleSingle entry descriptor */
{
    uint32 entryID; /* entry type: see list, 0 invalid */
    uint32 entryOffset; /* offset, in octets, from beginning */
                        /* of file to this entry's data */
    uint32 entryLength; /* length of data in octets */
}; /* ASEntry */

typedef struct ASEntry ASEntry;

/* Apple reserves the range of entry IDs from 1 to 0x7FFFFFFF.
 * Entry ID 0 is invalid.  The rest of the range is available
 * for applications to define their own entry types.  "Apple does
 * not arbitrate the use of the rest of the range."
 */

#define AS_DATA         1 /* data fork */
#define AS_RESOURCE     2 /* resource fork */
#define AS_REALNAME     3 /* File's name on home file system */
#define AS_COMMENT      4 /* standard Mac comment */
#define AS_ICONBW       5 /* Mac black & white icon */
#define AS_ICONCOLOR    6 /* Mac color icon */
        /*              7       /* not used */
#define AS_FILEDATES    8 /* file dates; create, modify, etc */
#define AS_FINDERINFO   9 /* Mac Finder info & extended info */
#define AS_MACINFO      10 /* Mac file info, attributes, etc */
#define AS_PRODOSINFO   11 /* Pro-DOS file info, attrib., etc */
#define AS_MSDOSINFO    12 /* MS-DOS file info, attributes, etc */
#define AS_AFPNAME      13 /* Short name on AFP server */
#define AS_AFPINFO      14 /* AFP file info, attrib., etc */

#define AS_AFPDIRID     15 /* AFP directory ID */

/* matrix of entry types and their usage:
 *
 *                   Macintosh    Pro-DOS    MS-DOS    AFP server
 *                   ---------    -------    ------    ----------
 *  1   AS_DATA         xxx         xxx       xxx         xxx
 *  2   AS_RESOURCE     xxx         xxx
 *  3   AS_REALNAME     xxx         xxx       xxx         xxx
 *
 *  4   AS_COMMENT      xxx
 *  5   AS_ICONBW       xxx
 *  6   AS_ICONCOLOR    xxx
 *
 *  8   AS_FILEDATES    xxx         xxx       xxx         xxx
 *  9   AS_FINDERINFO   xxx
 * 10   AS_MACINFO      xxx
 *
 * 11   AS_PRODOSINFO               xxx
 * 12   AS_MSDOSINFO                          xxx
 *
 * 13   AS_AFPNAME                                        xxx
 * 14   AS_AFPINFO                                        xxx
 * 15   AS_AFPDIRID                                       xxx
 */

/* entry ID 1, data fork of file - arbitrary length octet string */

/* entry ID 2, resource fork - arbitrary length opaque octet string;
 *              as created and managed by Mac O.S. resoure manager
 */

/* entry ID 3, file's name as created on home file system - arbitrary
 *              length octet string; usually short, printable ASCII
 */

/* entry ID 4, standard Macintosh comment - arbitrary length octet
 *              string; printable ASCII, claimed 200 chars or less
 */

/* This is probably a simple duplicate of the 128 octet bitmap
 * stored as the 'ICON' resource or the icon element from an 'ICN#'
 * resource.
 */

struct ASIconBW /* entry ID 5, standard Mac black and white icon */
{
    uint32 bitrow[32]; /* 32 rows of 32 1-bit pixels */
}; /* ASIconBW */

typedef struct ASIconBW ASIconBW;

/* entry ID 6, "standard" Macintosh color icon - several competing
 *              color icons are defined.  Given the copyright dates
 * of the Inside Macintosh volumes, the 'cicn' resource predominated
 * when the AppleSingle Developer's Note was written (most probable
 * candidate).  See Inside Macintosh, Volume V, pages 64 & 80-81 for
 * a description of 'cicn' resources.
 *
 * With System 7, Apple introduced icon families.  They consist of:
 *      large (32x32) B&W icon, 1-bit/pixel,    type 'ICN#',
 *      small (16x16) B&W icon, 1-bit/pixel,    type 'ics#',
 *      large (32x32) color icon, 4-bits/pixel, type 'icl4',
 *      small (16x16) color icon, 4-bits/pixel, type 'ics4',
 *      large (32x32) color icon, 8-bits/pixel, type 'icl8', and
 *      small (16x16) color icon, 8-bits/pixel, type 'ics8'.
 * If entry ID 6 is one of these, take your pick.  See Inside
 * Macintosh, Volume VI, pages 2-18 to 2-22 and 9-9 to 9-13, for
 * descriptions.
 */

/* entry ID 7, not used */

/* Times are stored as a "signed number of seconds before of after
 * 12:00 a.m. (midnight), January 1, 2000 Greenwich Mean Time (GMT).
 * Applications must convert to their native date and time
 * conventions." Any unknown entries are set to 0x80000000
 * (earliest reasonable time).
 */

struct ASFileDates      /* entry ID 8, file dates info */
{
    sint32 create; /* file creation date/time */
    sint32 modify; /* last modification date/time */
    sint32 backup; /* last backup date/time */
    sint32 access; /* last access date/time */
}; /* ASFileDates */

typedef struct ASFileDates ASFileDates;

/* See older Inside Macintosh, Volume II, page 115 for
 * PBGetFileInfo(), and Volume IV, page 155, for PBGetCatInfo().
 */

/* entry ID 9, Macintosh Finder info & extended info */
struct ASFinderInfo
{
    FInfo ioFlFndrInfo; /* PBGetFileInfo() or PBGetCatInfo() */
    FXInfo ioFlXFndrInfo; /* PBGetCatInfo() (HFS only) */
}; /* ASFinderInfo */

typedef struct ASFinderInfo ASFinderInfo;

struct ASMacInfo        /* entry ID 10, Macintosh file information */
{
    uchar8 filler[3]; /* filler, currently all bits 0 */
    uchar8 ioFlAttrib; /* PBGetFileInfo() or PBGetCatInfo() */
}; /* ASMacInfo */

typedef struct ASMacInfo ASMacInfo;

#define AS_PROTECTED    0x0002 /* protected bit */
#define AS_LOCKED       0x0001 /* locked bit */

/* NOTE: ProDOS-16 and GS/OS use entire fields.  ProDOS-8 uses low
 * order half of each item (low byte in access & filetype, low word
 * in auxtype); remainder of each field should be zero filled.
 */

struct ASProdosInfo     /* entry ID 11, ProDOS file information */
{
    uint16 access; /* access word */
    uint16 filetype; /* file type of original file */
    uint32 auxtype; /* auxiliary type of the orig file */
}; /* ASProDosInfo */

typedef struct ASProdosInfo ASProdosInfo;

/* MS-DOS file attributes occupy 1 octet; since the Developer Note
 * is unspecific, I've placed them in the low order portion of the
 * field (based on example of other ASMacInfo & ASProdosInfo).
 */

struct ASMsdosInfo      /* entry ID 12, MS-DOS file information */
{
    uchar8 filler; /* filler, currently all bits 0 */
    uchar8 attr; /* _dos_getfileattr(), MS-DOS */
                                /* interrupt 21h function 4300h */
}; /* ASMsdosInfo */

typedef struct ASMsdosInfo ASMsdosInfo;

#define AS_DOS_NORMAL   0x00 /* normal file (all bits clear) */
#define AS_DOS_READONLY 0x01 /* file is read-only */
#define AS_DOS_HIDDEN   0x02 /* hidden file (not shown by DIR) */
#define AS_DOS_SYSTEM   0x04 /* system file (not shown by DIR) */
#define AS_DOS_VOLID    0x08 /* volume label (only in root dir) */
#define AS_DOS_SUBDIR   0x10 /* file is a subdirectory */
#define AS_DOS_ARCHIVE  0x20 /* new or modified (needs backup) */

/* entry ID 13, short file name on AFP server - arbitrary length
 *              octet string; usualy printable ASCII starting with
 *              '!' (0x21)
 */

struct ASAfpInfo   /* entry ID 12, AFP server file information */
{
    uchar8 filler[3]; /* filler, currently all bits 0 */
    uchar8 attr; /* file attributes */
}; /* ASAfpInfo */

typedef struct ASAfpInfo ASAfpInfo;

#define AS_AFP_Invisible    0x01 /* file is invisible */
#define AS_AFP_MultiUser    0x02 /* simultaneous access allowed */
#define AS_AFP_System       0x04 /* system file */
#define AS_AFP_BackupNeeded 0x40 /* new or modified (needs backup) */

struct ASAfpDirId       /* entry ID 15, AFP server directory ID */
{
    uint32 dirid; /* file's directory ID on AFP server */
}; /* ASAfpDirId */

typedef struct ASAfpDirId ASAfpDirId;

/*
 * The format of an AppleSingle/AppleDouble header
 */
struct AppleSingle /* format of disk file */
{
    ASHeader header; /* AppleSingle header part */
    ASEntry  entry[1]; /* array of entry descriptors */
/* uchar8  filedata[];          /* followed by rest of file */
}; /* AppleSingle */

typedef struct AppleSingle AppleSingle;

/*
 * FINAL REMINDER: the Motorola 680x0 is a big-endian architecture!
 */

/* End of applefile.h */
