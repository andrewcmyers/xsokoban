/***********************************************************************
   You may wish to alter the following directory paths
***********************************************************************/
/**/
/* SCREENPATH: the name of the directioy where the screen file are held */
/**/
#ifndef SCREENPATH
#define SCREENPATH "/usr/tmp/xsokoban.v2/screens"
#endif

/**/
/* SAVEPATH: the name of the path where save files are held */
/*           Attention: Be sure that there are no other files with */
/*                      the name <username>.sav                    */
/**/
#ifndef SAVEPATH
#define SAVEPATH "/usr/tmp/xsokoban.v2/saves"
#endif

/* BITPATH: the full pathname to the bitmap file defaults. */
#ifndef BITPATH
#define BITPATH "/usr/tmp/xsokoban.v2/bitmaps/defaults"
#endif

/**/
/* LOCKPATH: temporary file which is created to ensure that no users */
/*           work with the scorefile at the same time                */
/**/
#ifndef LOCKFILE
#define LOCKFILE "/tmp/score.slock"
#endif

/**/
/* SCOREFILE: the full pathname of the score file */
/**/
#ifndef SCOREFILE
#define SCOREFILE "/usr/tmp/xsokoban.v2/sokoban.slock"
#endif

/**/
/* MAXUSERNAME: defines the maximum length of a system's user name */
/**/
#define MAXUSERNAME     32

/**/
/* MAXSCOREENTRIES: defines the maximum numner of entries in the scoretable */
/**/
#define MAXSCOREENTRIES 10000

/**/
/* SUPERUSER: defines the name of the game superuser */
/**/
#ifndef SUPERUSER
#define SUPERUSER "jt1o"
#endif

/**/
/* PASSWORD: defines the password necessary for creating a new score file */
/**/
#ifndef PASSWORD
#define PASSWORD "Greezooble"
#endif
