/***********************************************************************
   You may wish to alter the following directory paths
***********************************************************************/

#ifndef ROOTDIR
#define ROOTDIR "/usr/local/lib/xsokoban"
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
/* NUMBESTSCORES: the number of best scores kept for a given level
/**/
#define NUMBESTSCORES 5

/**/
/* MAXLEVELS: The number of levels for which best scores are kept
/**/
#define MAXLEVELS 50

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
/* ANYLEVEL: Allow any user to play any level and get a score for it */
/**/
#define ANYLEVEL

/**/
/* MAXSOLNRANK: The maximum solution rank for which an entry is retained */
/* in the score table. */
/**/
#define MAXSOLNRANK 5

/**/
/* STACKDEPTH: Number of previous positions remembered in the move stack
/**/
#define STACKDEPTH 1000
