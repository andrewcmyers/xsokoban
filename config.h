/***********************************************************************
   You may wish to alter the following directory paths.

   Note that the string concatenation performed below requires an
   ANSI-standard preprocessor. If you don't have one, you'll have to
   manually edit SCREENPATH, SAVEPATH, etc. to start with ROOTDIR.
***********************************************************************/

#ifndef ROOTDIR
#define ROOTDIR "."
/* I suggest "/usr/local/lib/xsokoban" as a value for this variable
   in the installed version, but you know best...
*/
#endif

/**/
/* SCREENPATH: the name of the directory where the screen files are held */
/**/
#ifndef SCREENPATH
#define SCREENPATH ROOTDIR "/screens"
#endif

/**/
/* SAVEPATH: the name of the path where save files are held */
/*           Attention: Be sure that there are no other files with */
/*                      the name <username>.sav                    */
/**/
#ifndef SAVEPATH
#define SAVEPATH ROOTDIR "/saves"
#endif

/* BITPATH: the full pathname to the bitmap file defaults. */
#ifndef BITPATH
#define BITPATH ROOTDIR "/bitmaps/defaults"
#endif

/**/
/* LOCKPATH: temporary file which is created to ensure that no users */
/*           work with the scorefile at the same time                */
/**/
#ifndef LOCKFILE
#define LOCKFILE ROOTDIR "/scores/lock"
#endif

/**/
/* SCOREFILE: the full pathname of the score file */
/**/
#ifndef SCOREFILE
#define SCOREFILE ROOTDIR "/scores/scores"
#endif

/**/
/* MAXUSERNAME: defines the maximum length of a system's user name */
/**/
#define MAXUSERNAME     32

/**/
/* MAXSCOREENTRIES: defines the maximum number of entries in the scoretable */
/**/
#define MAXSCOREENTRIES 10000

/**/
/* SUPERUSER: defines the name of the game superuser */
/**/
#ifndef SUPERUSER
#define SUPERUSER "andru"
#endif

/**/
/* PASSWORD: defines the password necessary for creating a new score file */
/**/
#ifndef PASSWORD
#define PASSWORD "score"
#endif

/**/
/* ANYLEVEL: Allow any user to play any level and enter a score for it */
/**/
#undef ANYLEVEL

/**/
/* MAXSOLNRANK: The maximum solution rank for which an entry is retained */
/* in the score table. */
/**/
#define MAXSOLNRANK 5

/**/
/* STACKDEPTH: Number of previous positions remembered in the move stack */
/**/
#define STACKDEPTH 1000
