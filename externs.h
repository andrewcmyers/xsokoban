#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xresource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <malloc.h>

#ifdef NEED_NETINET_IN
#include <netinet/in.h>
#endif

#ifdef NEED_NH
#include <net/nh.h>
#endif

#ifdef NEED_ENDIAN
#include <machine/endian.h>
#endif

#ifdef NEED_BYTEORDER
#include <sys/byteorder.h>
#endif

#include "config_local.h"

#if !defined(GETPASS_PROTO)
extern char *getpass(char *);
#endif

#if !defined(FPRINTF_PROTO)
extern int fprintf(FILE *, const char *, ...);
#endif

#if !defined(FCLOSE_PROTO)
extern int fclose(FILE *);
#endif

#if !defined(TIME_PROTO)
extern time_t time(time_t *);
#endif

#if !defined(MKTEMP_PROTO)
extern char *mktemp(char *tempfile);
#endif

#if !defined(PERROR_PROTO)
extern void perror(char *);
#endif

#if !defined(RENAME_PROTO)
extern int rename(char *from, char *to);
#endif

#if !defined(STRDUP_PROTO)
extern char *strdup(const char *);
#endif

#if defined(HAS_USLEEP)
#if !defined(USLEEP_PROTO)
extern void usleep(unsigned);
#endif
#endif

#if !defined(LOCALTIME_PROTO)
extern struct tm *localtime(time_t *);
#endif

#if !defined(BZERO_PROTO)
extern void bzero(char *, int);
#endif

int fsync(int);

#ifndef EXIT_FAILURE
#define EXIT_FAILURE -1
#endif

/* The boolean typedef */
typedef enum { _false_ = 0, _true_ = 1 } Boolean;

/* stuff from resources.c */
extern char *GetDatabaseResource(XrmDatabase, char *);
extern char *GetResource(char *);
extern Boolean StringToBoolean(char *);
extern Boolean GetColorResource(char *, unsigned long *);
extern XFontStruct *GetFontResource(char *);
extern unsigned long GetColorOrDefault(Display *, char *,
				       int, char *, Boolean);

/* stuff from play.c */
extern short Play(void);
/* Play a particular level. All we do here is wait for input, and
   dispatch to appropriate routines to deal with it nicely.
*/

extern Boolean Verify(int, char *);
/* Determine whether the move sequence solves
   the current level. Return "_true_" if so.  Set "moves" and "pushes"
   appropriately.

   "moveseqlen" must be the number of characters in "moveseq".

   The format of "moveseq" is as described in "ApplyMoves".
*/


/* stuff from screen.c */
extern short ReadScreen(void);

/* stuff from save.c */
extern short SaveGame(void);
extern short RestoreGame(void);

/* stuff from scoredisp.c */
extern short DisplayScores_(Display *, Window, short *);
/* Display scores. Return E_ENDGAME if user wanted to quit from here.
   If user middle-clicked on a level, and "newlev" is non-zero, put
   the level clicked-on into "newlev".
   */

/* stuff from qtelnet.c */
extern char *qtelnet(char *host, int port, char *msg);
/* Open a TCP-IP connection to machine "hostname" on port "port", and
   send it the text in "msg". Return all output from the connection
   in a newly-allocated string that must be freed to reclaim its
   storage.

   Return 0 on failure.
*/
